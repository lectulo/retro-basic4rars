#include "basic.h"

Token   tokens[MAX_TOKENS];
int32_t ntokens;
Line    lines[MAX_LINES];
int32_t nlines;
char    strpool[STR_POOL];

static int32_t poolused;

static uint8_t used_bare[ARR_SLOTS];
static uint8_t used_array[ARR_SLOTS];

static const char* keywords[K_COUNT] = {
    "AND", "BASE", "DATA", "DEF", "DIM", "ELSE", "END", "FOR",
    "GO", "GOSUB", "GOTO", "IF", "INPUT", "LET", "NEXT", "NOT",
    "ON", "OPTION", "OR", "PRINT", "RANDOMIZE", "READ", "REM",
    "RESTORE", "RETURN", "SPC", "STEP", "STOP", "SUB", "TAB",
    "THEN", "TO"
};

static const char* builtins[F_COUNT] = {
    "ABS", "ASC", "ATN", "COS", "EXP", "INT", "LEN", "LOG",
    "RND", "SGN", "SIN", "SQR", "TAN", "VAL"
};

static int32_t is_digit(char c) { return c >= '0' && c <= '9'; }

static int32_t is_upper(char c) { return c >= 'A' && c <= 'Z'; }

static char upcase(char c)
{
    return (c >= 'a' && c <= 'z') ? (char)(c - 'a' + 'A') : c;
}

static int32_t cur_line;

static void emit(int32_t kind, int32_t aux, int32_t slot, float num)
{
    Token* t;

    if (ntokens >= MAX_TOKENS) err_die(E_C15, 0, 0);

    t = &tokens[ntokens++];
    t->kind = (uint8_t)kind;
    t->aux  = (uint8_t)aux;
    t->slot = (uint16_t)slot;
    t->num  = num;
}

static int32_t pool_put(const char* s, int32_t n)
{
    int32_t off = poolused;
    int32_t i;

    if (poolused + n + 1 > STR_POOL) err_die(E_C15, 0, 0);

    for (i = 0; i < n; i++) strpool[poolused++] = s[i];
    strpool[poolused++] = '\0';

    return off;
}

static int32_t scan_string(const char* p, int32_t n, int32_t i)
{
    int32_t start = ++i;

    while (i < n && p[i] != '"') i++;
    if (i == n) emit(T_BAD, 0, 0, 0.0f);
    else emit(T_STR, 0, pool_put(&p[start], i - start), 0.0f);

    return (i < n) ? i + 1 : i;
}

static int32_t scan_data(const char* p, int32_t n, int32_t i)
{
    for (;;)
    {
        int32_t start;
        int32_t stop;
        float value;

        while (i < n && p[i] == ' ') i++;
        if (i >= n || p[i] == ':') return i;

        if (p[i] == '"')
        {
            i = scan_string(p, n, i);
        }
        else
        {
            int32_t sign = 1;
            int32_t original;
            int32_t used;

            original = start = i;
            while (i < n && p[i] != ',' && p[i] != ':') i++;

            stop = i;
            while (stop > start && p[stop - 1] == ' ') stop--;

            if (stop > start && (p[start] == '+' || p[start] == '-'))
            {
                if (p[start] == '-') sign = -1;
                start++;
            }

            used = str_scan_num(&p[start], stop - start, &value);
            if (used < 0) err_die(E_C16, cur_line, 0);
            if (used > 0 && used == stop - start)
            {

                emit(T_NUM, 0, pool_put(&p[original], stop - original),
                     sign < 0 ? -value : value);
            }
            else
            {
                int32_t k;
                int32_t off = pool_put(&p[original], stop - original);
                for (k = 0; k < stop - original; k++)
                {
                    if (p[original + k] == '"') err_die(E_C01, cur_line, 0);
                    strpool[off + k] = upcase(strpool[off + k]);
                }
                emit(T_STR, 0, off, 0.0f);
            }
        }

        while (i < n && p[i] == ' ') i++;
        if (i < n && p[i] == ',')
        {
            emit(T_PUNCT, ',', 0, 0.0f);
            i++;
            continue;
        }

        return i;
    }
}

static int32_t scan_word(const char* p, int32_t n, int32_t i, char* word,
                         int32_t* len)
{
    int32_t k = 0;

    while (i < n && k < 15)
    {
        char c = upcase(p[i]);

        if (!is_upper(c) && !is_digit(c)) break;
        word[k++] = c;
        i++;
    }
    word[k] = '\0';
    *len = k;

    return i;
}

static int32_t lookup(const char** table, int32_t count, const char* word)
{
    int32_t i;

    for (i = 0; i < count; i++)
    {
        if (str_eq(table[i], word)) return i;
    }

    return -1;
}

static void lex_body(const char* p, int32_t n)
{
    char word[16];
    int32_t len;
    int32_t i = 0;

    while (i < n)
    {
        char c = p[i];

        if (c == ' ' || c == '\t')
        {
            i++;
            continue;
        }

        if (is_digit(c) || (c == '.' && i + 1 < n && is_digit(p[i + 1])))
        {
            float value;
            int32_t used = str_scan_num(&p[i], n - i, &value);

            if (used < 0) err_die(E_C16, cur_line, 0);

            emit(T_NUM, 0, 0, value);
            i += used;
            continue;
        }

        if (c == '"')
        {
            i = scan_string(p, n, i);
            continue;
        }

        if (is_upper(upcase(c)))
        {
            int32_t code;

            i = scan_word(p, n, i, word, &len);

            if (str_eq(word, "GO"))
            {
                char next[16];
                int32_t nlen;
                int32_t j = i;

                while (j < n && p[j] == ' ') j++;
                j = scan_word(p, n, j, next, &nlen);

                if (str_eq(next, "TO") || str_eq(next, "SUB"))
                {
                    emit(T_KW, next[0] == 'T' ? K_GOTO : K_GOSUB, 0, 0.0f);
                    i = j;
                    continue;
                }
            }

            code = lookup(keywords, K_COUNT, word);
            if (code >= 0)
            {
                emit(T_KW, code, 0, 0.0f);

                if (code == K_REM) return;

                if (code == K_DATA) i = scan_data(p, n, i);
                continue;
            }

            code = lookup(builtins, F_COUNT, word);
            if (code >= 0)
            {
                emit(T_FUNC, code, 0, 0.0f);
                continue;
            }

            if (len == 3 && word[0] == 'F' && word[1] == 'N' && is_upper(word[2]))
            {
                emit(T_FN, 0, word[2] - 'A', 0.0f);
                continue;
            }

            if (len == 1 && is_upper(word[0]))
            {
                int32_t letter = word[0] - 'A';
                int32_t j = i;

                if (i < n && p[i] == '$')
                {
                    emit(T_STRVAR, 0, letter, 0.0f);
                    i++;
                    continue;
                }

                while (j < n && p[j] == ' ') j++;
                if (j < n && p[j] == '(')
                {
                    used_array[letter] = 1;
                    emit(T_ARRAY, 0, letter, 0.0f);
                    continue;
                }

                used_bare[letter] = 1;
                emit(T_NUMVAR, 0, letter * 11, 0.0f);
                continue;
            }

            if (len == 2 && is_upper(word[0]) && is_digit(word[1]))
            {
                emit(T_NUMVAR, 0, (word[0] - 'A') * 11 + (word[1] - '0') + 1,
                     0.0f);
                continue;
            }

            emit(T_BAD, 0, 0, 0.0f);
            continue;
        }

        if (i + 1 < n)
        {
            int32_t rel = 0;

            if (c == '<' && p[i + 1] == '=') rel = P_LE;
            else if (c == '<' && p[i + 1] == '>') rel = P_NE;
            else if (c == '>' && p[i + 1] == '=') rel = P_GE;

            if (rel != 0)
            {
                emit(T_PUNCT, rel, 0, 0.0f);
                i += 2;
                continue;
            }
        }

        emit(T_PUNCT, (unsigned char)c, 0, 0.0f);
        i++;
    }
}

void lex_program(const char* src, int32_t len)
{
    int32_t pos = 0;
    int32_t i;

    ntokens = 0;
    nlines = 0;
    poolused = 0;
    cur_line = 0;

    for (i = 0; i < ARR_SLOTS; i++)
    {
        used_bare[i] = 0;
        used_array[i] = 0;
    }

    while (pos < len)
    {
        int32_t start = pos;
        int32_t stop;
        int32_t number = 0;
        int32_t digits = 0;
        int32_t j;

        while (pos < len && src[pos] != '\n') pos++;

        stop = pos;
        if (stop > start && src[stop - 1] == '\r') stop--;
        if (pos < len) pos++;

        if (stop - start > MAX_LINE_LEN) err_die(E_C12, nlines + 1, 0);

        j = start;
        while (j < stop && (src[j] == ' ' || src[j] == '\t')) j++;
        if (j >= stop) continue;
        if (j != start) err_die(E_C01, 0, 0);

        while (j < stop && is_digit(src[j]))
        {

            if (number < 100000) number = number * 10 + (src[j] - '0');
            digits++;
            j++;
        }

        if (digits == 0 || number < 1 || number > 9999) err_die(E_C02, number, 0);

        if (nlines > 0)
        {
            if (number == lines[nlines - 1].number) err_die(E_C03, number, 0);
            if (number < lines[nlines - 1].number) err_die(E_C14, number, 0);
        }

        if (nlines >= MAX_LINES) err_die(E_C15, 0, 0);

        cur_line = number;
        lines[nlines].number = (uint16_t)number;
        lines[nlines].start = (uint16_t)ntokens;
        nlines++;

        lex_body(&src[j], stop - j);
        emit(T_EOL, 0, 0, 0.0f);
    }

    for (i = 0; i < ARR_SLOTS; i++)
    {
        if (used_bare[i] && used_array[i]) err_die(E_C07, 0, 0);
    }
}

static void dump_num(float v)
{
    int32_t whole;
    int32_t i;

    if (v < 0.0f)
    {
        port_putc('-');
        v = -v;
    }

    whole = (int32_t)v;
    out_putn(whole);
    v -= (float)whole;

    if (v == 0.0f) return;

    port_putc('.');
    for (i = 0; i < 6; i++)
    {
        v *= 10.0f;
        whole = (int32_t)v;
        port_putc((char)('0' + whole));
        v -= (float)whole;
    }
}

static void dump_punct(int32_t aux)
{
    if (aux == P_LE) out_puts("<=");
    else if (aux == P_GE) out_puts(">=");
    else if (aux == P_NE) out_puts("<>");
    else port_putc((char)aux);
}

void lex_dump(void)
{
    int32_t i;
    int32_t k;

    for (i = 0; i < nlines; i++)
    {
        out_putn(lines[i].number);
        out_puts(":\n");

        for (k = lines[i].start; tokens[k].kind != T_EOL; k++)
        {
            const Token* t = &tokens[k];

            out_puts("  ");
            switch (t->kind)
            {
            case T_KW:     out_puts("KW ");     out_puts(keywords[t->aux]); break;
            case T_FUNC:   out_puts("FUNC ");   out_puts(builtins[t->aux]); break;
            case T_FN:     out_puts("FN ");     out_putn(t->slot); break;
            case T_NUM:    out_puts("NUM ");    dump_num(t->num); break;
            case T_STR:    out_puts("STR \"");  out_puts(&strpool[t->slot]);
                           port_putc('"'); break;
            case T_NUMVAR: out_puts("NUMVAR "); out_putn(t->slot); break;
            case T_STRVAR: out_puts("STRVAR "); out_putn(t->slot); break;
            case T_ARRAY:  out_puts("ARRAY ");  out_putn(t->slot); break;
            case T_PUNCT:  out_puts("PUNCT ");  dump_punct(t->aux); break;
            default:       out_puts("BAD"); break;
            }
            port_putc('\n');
        }
    }
}
