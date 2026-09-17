#include "basic.h"

typedef struct
{
    uint32_t body;
    uint32_t end;
    int32_t line;
    int32_t slot;
    float limit;
    float step;
} Loop;

static Loop loops[FOR_DEPTH];
static int32_t nloops;
static uint32_t returns[GOSUB_DEPTH];
static int32_t loop_base[GOSUB_DEPTH];
static int32_t nreturns;

static void expect(Cur* c, int32_t kind, int32_t aux)
{
    if (cur_at(c)->kind != kind || cur_at(c)->aux != aux)
        err_die(E_C01, err_line, 0);
    c->p++;
}

static void finish(const Cur* c)
{
    if (!cur_end(c)) err_die(E_C01, err_line, 0);
}

static void skip(Cur* c)
{
    while (!cur_end(c)) c->p++;
}

static int32_t base(void)
{
    return nreturns ? loop_base[nreturns - 1] : 0;
}

static void jump(Cur* c, int32_t number, int32_t sub)
{
    uint32_t dest = lines[prog_line_index(number)].start;
    if (sub)
    {
        if (nreturns == GOSUB_DEPTH) err_die(E_R09, err_line, 0);
        returns[nreturns] = c->p;
        loop_base[nreturns++] = nloops;
    }
    else
    {

        while (nloops > base()
               && (dest < loops[nloops - 1].body || dest > loops[nloops - 1].end))
            nloops--;
    }
    c->p = dest;
}

static void assignment(Cur* c)
{
    if (cur_at(c)->kind == T_STRVAR)
    {
        int32_t slot = cur_at(c)->slot;
        c->p++;
        expect(c, T_PUNCT, '=');
        var_str_set(slot, expr_str(c));
    }
    else
    {
        float* dest = expr_ref(c);
        expect(c, T_PUNCT, '=');
        *dest = expr_num(c);
    }
}

static void print(Cur* c)
{
    int32_t newline = 1;
    while (!cur_end(c))
    {

        if (cur_punct(c, ',') || cur_punct(c, ';'))
        {
            if (cur_punct(c, ',')) fmt_comma();
            c->p++;
            newline = 0;
            continue;
        }
        if (cur_kw(c, K_TAB) || cur_kw(c, K_SPC))
        {
            int32_t tab = cur_kw(c, K_TAB);
            float x;
            c->p++;
            expect(c, T_PUNCT, '(');
            x = expr_num(c);
            expect(c, T_PUNCT, ')');
            if (tab) fmt_tab(x);
            else fmt_spc(x);
        }
        else if (expr_is_str(c)) fmt_item(expr_str(c));
        else
        {
            char buf[FMT_BUF];
            buf[fmt_num(expr_num(c), buf)] = '\0';
            fmt_item(buf);
        }
        newline = 1;
        if (!cur_punct(c, ',') && !cur_punct(c, ';')) break;
        if (cur_punct(c, ',')) fmt_comma();
        c->p++;
        newline = 0;
    }
    if (newline) fmt_nl();
}

static void read_data(Cur* c)
{
    for (;;)
    {
        int32_t slot = -1;
        float* dest = NULL;
        const Token* t;
        if (cur_at(c)->kind == T_STRVAR)
        {
            slot = cur_at(c)->slot;
            c->p++;
        }
        else dest = expr_ref(c);
        t = prog_data_next();
        if (slot < 0 && t->kind != T_NUM) err_die(E_C10, err_line, 0);
        if (slot >= 0) var_str_set(slot, &strpool[t->slot]);
        else *dest = t->num;
        if (!cur_punct(c, ',')) break;
        c->p++;
    }
}

#define INPUT_ITEMS     (MAX_LINE_LEN / 2)
#define INPUT_BUF       (INPUT_ITEMS * (STR_BUF + 4))

static void input(Cur* c)
{
    static uint32_t refs[INPUT_ITEMS];
    static int32_t slots[INPUT_ITEMS];
    static float values[INPUT_ITEMS];
    static char* strings[INPUT_ITEMS];
    static char buf[INPUT_BUF];
    int32_t count = 0;
    int32_t i;

    for (;;)
    {
        if (count == INPUT_ITEMS) err_die(E_C15, 0, 0);
        slots[count] = -1;
        if (cur_at(c)->kind == T_STRVAR)
        {
            slots[count] = cur_at(c)->slot;
            c->p++;
        }
        else
        {
            int32_t array = cur_at(c)->kind == T_ARRAY;
            if (!array && cur_at(c)->kind != T_NUMVAR) err_die(E_C01, err_line, 0);
            refs[count] = c->p++;
            if (array)
            {
                int32_t depth = 1;
                expect(c, T_PUNCT, '(');
                while (depth)
                {
                    if (cur_end(c)) err_die(E_C01, err_line, 0);
                    if (cur_punct(c, '(')) depth++;
                    if (cur_punct(c, ')')) depth--;
                    c->p++;
                }
            }
        }
        count++;
        if (!cur_punct(c, ',')) break;
        c->p++;
    }
    finish(c);

    for (;;)
    {
        char* p = buf;
        int32_t ok = 1;
        fmt_item("? ");
        port_gets(buf, INPUT_BUF);

        i = str_len(buf);
        if (i == 0 || buf[i - 1] != '\n') ok = 0;
        else buf[--i] = '\0';
        if (i && buf[i - 1] == '\r') buf[i - 1] = '\0';

        for (i = 0; ok && i < count; i++)
        {
            char* start;
            char* end;
            int32_t quoted;
            while (*p == ' ') p++;
            quoted = *p == '"';
            if (quoted) p++;
            start = p;
            if (quoted)
            {
                while (*p && *p != '"') p++;
                if (*p != '"') { ok = 0; break; }
                end = p++;
                while (*p == ' ') p++;
            }
            else
            {
                while (*p && *p != ',')
                {

                    if (*p == ';' || *p == '"') ok = 0;
                    p++;
                }
                end = p;
                while (end > start && end[-1] == ' ') end--;
            }
            if ((i + 1 < count && *p != ',') || (i + 1 == count && *p)) ok = 0;
            if (*p == ',') p++;
            *end = '\0';
            if (slots[i] >= 0)
            {
                strings[i] = start;
                if (end - start > STR_LEN || (!quoted && end == start)) ok = 0;
            }
            else
            {
                int32_t sign = 1;
                int32_t used;
                if (*start == '+' || *start == '-')
                {
                    if (*start == '-') sign = -1;
                    start++;
                }
                used = str_scan_num(start, (int32_t)(end - start), &values[i]);
                if (quoted || used <= 0 || used != end - start) ok = 0;
                if (sign < 0) values[i] = -values[i];
            }
        }
        if (ok) break;
        fmt_item("REDO FROM START");
        fmt_nl();
    }
    for (i = 0; i < count; i++)
    {
        if (slots[i] >= 0) var_str_set(slots[i], strings[i]);
        else
        {

            Cur target = {refs[i]};
            *expr_ref(&target) = values[i];
        }
    }
}

static void conditional(Cur* c)
{
    int32_t yes = expr_cond(c);
    expect(c, T_KW, K_THEN);
    if (!yes)
    {
        int32_t depth = 0;
        while (cur_at(c)->kind != T_EOL)
        {
            if (cur_kw(c, K_IF)) depth++;
            if (cur_kw(c, K_ELSE))
            {
                if (depth == 0) { c->p++; break; }
                depth--;
            }
            c->p++;
        }
    }
    if (cur_at(c)->kind == T_NUM)
    {
        int32_t number = (int32_t)cur_at(c)->num;
        c->p++;
        finish(c);
        jump(c, number, 0);
    }
}

static void on_branch(Cur* c)
{
    int32_t index = math_round(expr_num(c));
    int32_t sub = cur_kw(c, K_GOSUB);
    int32_t count = 0;
    int32_t target = 0;
    if (!sub && !cur_kw(c, K_GOTO)) err_die(E_C01, err_line, 0);
    c->p++;
    for (;;)
    {
        if (cur_at(c)->kind != T_NUM) err_die(E_C01, err_line, 0);
        if (++count == index) target = (int32_t)cur_at(c)->num;
        c->p++;
        if (!cur_punct(c, ',')) break;
        c->p++;
    }
    finish(c);
    if (index < 1 || index > count) err_die(E_R08, err_line, 0);
    jump(c, target, sub);
}

static int32_t done(const Loop* f)
{
    float x = var_num_get(f->slot);

    if (f->step > 0.0f) return x > f->limit;
    if (f->step < 0.0f) return x < f->limit;
    return 0;
}

static void for_loop(Cur* c)
{
    Loop* f;
    Cur end;
    float start;
    int32_t depth = 0;
    int32_t i;
    if (nloops == FOR_DEPTH) err_die(E_C15, 0, 0);
    f = &loops[nloops];
    if (cur_at(c)->kind != T_NUMVAR) err_die(E_C01, err_line, 0);
    f->slot = cur_at(c)->slot;
    f->line = err_line;
    for (i = base(); i < nloops; i++)
        if (loops[i].slot == f->slot) err_die(E_C01, err_line, 0);
    c->p++;
    expect(c, T_PUNCT, '=');
    start = expr_num(c);
    expect(c, T_KW, K_TO);
    f->limit = expr_num(c);
    f->step = 1.0f;
    if (cur_kw(c, K_STEP))
    {
        c->p++;
        f->step = expr_num(c);
    }
    finish(c);
    var_num_set(f->slot, start);
    f->body = c->p;
    end.p = c->p;

    for (; end.p < (uint32_t)ntokens; end.p++)
    {
        if (cur_kw(&end, K_FOR)) depth++;
        if (cur_kw(&end, K_NEXT) && depth-- == 0) break;
    }
    if (end.p == (uint32_t)ntokens) err_die(E_C01, err_line, 0);
    f->end = end.p;
    if (done(f))
    {
        c->p = end.p + 1;
        if (cur_at(c)->kind == T_NUMVAR)
        {
            if (cur_at(c)->slot != f->slot) err_die(E_R05, err_line, f->line);
            c->p++;
        }
        finish(c);
    }
    else nloops++;
}

static void next_loop(Cur* c)
{
    Loop* f;
    float x;
    if (nloops == base()) err_die(E_R06, err_line, 0);
    f = &loops[nloops - 1];
    if (cur_at(c)->kind == T_NUMVAR)
    {
        if (cur_at(c)->slot != f->slot) err_die(E_R05, err_line, f->line);
        c->p++;
    }
    finish(c);
    x = var_num_get(f->slot) + f->step;
    if (x > NUM_MAX || x < -NUM_MAX) err_die(E_R14, err_line, 0);
    var_num_set(f->slot, x);
    if (done(f)) nloops--;
    else c->p = f->body;
}

void exec_program(void)
{
    Cur c = {0};
    int32_t line = 0;
    nloops = 0;
    nreturns = 0;
    while (c.p < (uint32_t)ntokens)
    {
        int32_t code;
        if (cur_kw(&c, K_ELSE))
            while (cur_at(&c)->kind != T_EOL) c.p++;
        if (cur_at(&c)->kind == T_EOL || cur_punct(&c, ':')) { c.p++; continue; }
        while (line > 0 && c.p < lines[line].start) line--;
        while (line + 1 < nlines && c.p >= lines[line + 1].start) line++;
        err_line = lines[line].number;
        if (cur_at(&c)->kind != T_KW)
        {
            assignment(&c);
            finish(&c);
            continue;
        }
        code = cur_at(&c)->aux;
        c.p++;
        switch (code)
        {
        case K_LET: assignment(&c); break;
        case K_PRINT: print(&c); break;
        case K_READ: read_data(&c); break;
        case K_INPUT: input(&c); break;
        case K_RESTORE: prog_data_restore(); break;
        case K_RANDOMIZE: math_randomize(); break;
        case K_DATA: case K_DIM: case K_DEF: case K_OPTION: case K_REM:
            skip(&c);
            break;
        case K_IF: conditional(&c); continue;
        case K_ON: on_branch(&c); continue;
        case K_FOR: for_loop(&c); continue;
        case K_NEXT: next_loop(&c); continue;
        case K_GOTO: case K_GOSUB:
        {
            int32_t number = (int32_t)cur_at(&c)->num;
            c.p++;
            finish(&c);
            jump(&c, number, code == K_GOSUB);
            continue;
        }
        case K_RETURN:
            finish(&c);
            if (!nreturns) err_die(E_R07, err_line, 0);
            nloops = loop_base[--nreturns];
            c.p = returns[nreturns];
            continue;
        case K_END: case K_STOP:
            finish(&c);
            return;
        default: err_die(E_C01, err_line, 0);
        }
        finish(&c);
    }
}
