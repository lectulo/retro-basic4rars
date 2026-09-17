#include "basic.h"

static uint32_t data_idx[MAX_DATA];
static int32_t  ndata;
static int32_t  data_pos;

static int32_t option_seen;
static int32_t dim_seen;
static int32_t array_seen;

static int32_t end_line;
static int32_t end_last;
static int32_t end_count;

int32_t prog_line_index(int32_t number)
{
    int32_t lo = 0;
    int32_t hi = nlines - 1;

    while (lo <= hi)
    {
        int32_t mid = (lo + hi) / 2;

        if (lines[mid].number == number) return mid;
        if (lines[mid].number < number) lo = mid + 1;
        else hi = mid - 1;
    }

    return -1;
}

void prog_data_restore(void)
{
    data_pos = 0;
}

const Token* prog_data_next(void)
{
    if (data_pos >= ndata) err_die(E_R02, err_line, 0);

    return &tokens[data_idx[data_pos++]];
}

static int32_t integer(Cur* c)
{
    float x = cur_at(c)->num;
    if (cur_at(c)->kind != T_NUM || x < 0.0f || x > 9999.0f
        || x != math_floor(x)) err_die(E_C01, err_line, 0);
    c->p++;
    return (int32_t)x;
}

static void check_target(Cur* c)
{
    int32_t number = integer(c);
    if (prog_line_index(number) < 0) err_die(E_C04, number, err_line);
}

static void note(const Token* t)
{
    if (t->kind == T_FN && !expr_def_exists(t->slot)) err_die(E_C09, err_line, 0);
    if (t->kind == T_ARRAY) array_seen = 1;
}

static void skip_rest(Cur* c)
{
    while (!cur_end(c))
    {
        note(cur_at(c));
        c->p++;
    }
}

static void do_data(Cur* c)
{
    for (;;)
    {
        const Token* t = cur_at(c);

        if (t->kind != T_NUM && t->kind != T_STR) err_die(E_C01, err_line, 0);
        if (ndata >= MAX_DATA) err_die(E_C15, 0, 0);

        data_idx[ndata++] = c->p;
        c->p++;

        if (!cur_punct(c, ',')) break;
        c->p++;
    }
}

static void do_dim(Cur* c)
{
    for (;;)
    {
        int32_t letter;
        int32_t d1;
        int32_t d2 = 0;
        int32_t ndim = 1;

        if (cur_at(c)->kind != T_ARRAY) err_die(E_C01, err_line, 0);
        letter = cur_at(c)->slot;
        c->p++;

        if (!cur_punct(c, '(')) err_die(E_C01, err_line, 0);
        c->p++;

        d1 = integer(c);

        if (cur_punct(c, ','))
        {
            c->p++;
            d2 = integer(c);
            ndim = 2;
        }

        if (!cur_punct(c, ')')) err_die(E_C01, err_line, 0);
        c->p++;

        var_array_dim(letter, d1, d2, ndim);
        dim_seen = 1;

        if (!cur_punct(c, ',')) break;
        c->p++;
    }
}

static void do_option(Cur* c)
{
    int32_t base;

    if (!cur_kw(c, K_BASE)) err_die(E_C01, err_line, 0);
    c->p++;

    base = integer(c);
    if (base != 0 && base != 1) err_die(E_C01, err_line, 0);

    if (option_seen || dim_seen || array_seen) err_die(E_C11, 0, 0);
    option_seen = 1;
    var_set_base(base);
}

static void do_def(Cur* c)
{
    int32_t letter;
    int32_t param = -1;
    uint32_t body;

    if (cur_at(c)->kind != T_FN) err_die(E_C01, err_line, 0);
    letter = cur_at(c)->slot;
    if (expr_def_exists(letter)) err_die(E_C01, err_line, 0);
    c->p++;

    if (cur_punct(c, '('))
    {
        c->p++;
        if (cur_at(c)->kind != T_NUMVAR) err_die(E_C01, err_line, 0);
        param = cur_at(c)->slot;
        c->p++;
        if (!cur_punct(c, ')')) err_die(E_C01, err_line, 0);
        c->p++;
    }

    if (!cur_punct(c, '=')) err_die(E_C01, err_line, 0);
    c->p++;

    body = c->p;
    skip_rest(c);

    expr_def(letter, param, body);
}

static void statement_list(Cur* c, int32_t line);

static void branch(Cur* c, int32_t line)
{
    if (cur_at(c)->kind == T_NUM)
    {
        check_target(c);
        if (cur_at(c)->kind != T_EOL && !cur_kw(c, K_ELSE))
            err_die(E_C01, err_line, 0);
        return;
    }

    if (cur_end(c)) err_die(E_C01, err_line, 0);
    statement_list(c, line);
}

static void do_if(Cur* c, int32_t line)
{
    while (!cur_end(c) && !cur_kw(c, K_THEN))
    {
        note(cur_at(c));
        c->p++;
    }

    if (!cur_kw(c, K_THEN)) err_die(E_C01, err_line, 0);
    c->p++;

    branch(c, line);

    if (cur_kw(c, K_ELSE))
    {
        c->p++;
        branch(c, line);
    }
}

static void do_on(Cur* c)
{
    while (!cur_end(c) && !cur_kw(c, K_GOTO) && !cur_kw(c, K_GOSUB))
    {
        note(cur_at(c));
        c->p++;
    }

    if (cur_end(c)) err_die(E_C01, err_line, 0);
    c->p++;

    for (;;)
    {
        check_target(c);

        if (!cur_punct(c, ',')) break;
        c->p++;
    }
}

static void statement(Cur* c, int32_t line)
{
    const Token* t = cur_at(c);

    if (t->kind != T_KW)
    {
        skip_rest(c);
        return;
    }

    c->p++;

    switch (t->aux)
    {
    case K_DATA:    do_data(c);   break;
    case K_DIM:     do_dim(c);    break;
    case K_OPTION:  do_option(c); break;
    case K_DEF:     do_def(c);    break;

    case K_IF:      do_if(c, line); return;

    case K_ON:      do_on(c);     break;

    case K_GOTO:
    case K_GOSUB:
        check_target(c);
        break;

    case K_GO:
        if (!cur_kw(c, K_TO) && !cur_kw(c, K_SUB)) err_die(E_C01, err_line, 0);
        c->p++;
        check_target(c);
        break;

    case K_END:
        end_count++;
        end_line = line;
        end_last = cur_at(c)->kind == T_EOL;
        break;

    default:
        skip_rest(c);
        break;
    }

    if (!cur_end(c)) err_die(E_C01, err_line, 0);
}

static void statement_list(Cur* c, int32_t line)
{
    for (;;)
    {
        if (cur_end(c)) err_die(E_C01, err_line, 0);

        statement(c, line);

        if (!cur_punct(c, ':')) return;
        c->p++;
    }
}

void prog_pass0(void)
{
    int32_t i;

    ndata = 0;
    data_pos = 0;
    option_seen = 0;
    dim_seen = 0;
    array_seen = 0;
    end_line = -1;
    end_last = 0;
    end_count = 0;

    var_reset();
    expr_def_reset();

    for (i = 0; i < nlines; i++)
    {
        Cur c;

        err_line = lines[i].number;
        c.p = lines[i].start;

        statement_list(&c, i);

        if (cur_kw(&c, K_ELSE)) err_die(E_C13, err_line, 0);
        if (cur_at(&c)->kind != T_EOL) err_die(E_C01, err_line, 0);
    }

    if (end_count == 0) err_die(E_C05, 0, 0);
    if (end_count > 1 || end_line != nlines - 1 || !end_last)
        err_die(E_C06, 0, 0);
    err_line = 0;
}
