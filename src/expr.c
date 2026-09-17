#include "basic.h"

static uint32_t fn_body[ARR_SLOTS];
static int16_t fn_param[ARR_SLOTS];
static uint8_t fn_defined[ARR_SLOTS];
static int32_t local_slot;
static float local_value;

void expr_def_reset(void)
{
    int32_t i;

    for (i = 0; i < ARR_SLOTS; i++) fn_defined[i] = 0;
    local_slot = 0;
}

int32_t expr_def_exists(int32_t letter)
{
    return fn_defined[letter];
}

void expr_def(int32_t letter, int32_t param_slot, uint32_t body)
{
    fn_body[letter] = body;
    fn_param[letter] = (int16_t)param_slot;
    fn_defined[letter] = 1;
}

static void expect(Cur* c, int32_t ch)
{
    if (!cur_punct(c, ch)) err_die(E_C01, err_line, 0);
    c->p++;
}

static float chk(float x)
{
    if (x != x || x > NUM_MAX || x < -NUM_MAX) err_die(E_R14, err_line, 0);

    return x;
}

static float builtin(Cur* c, int32_t code)
{
    float x;

    c->p++;

    if (code == F_RND) return math_rnd();

    if (code == F_LEN || code == F_ASC || code == F_VAL)
    {
        const char* s;

        expect(c, '(');
        s = expr_str(c);
        expect(c, ')');

        if (code == F_LEN) return (float)str_len(s);

        if (code == F_ASC)
        {
            if (s[0] == '\0') err_die(E_R13, err_line, 0);

            return (float)(unsigned char)s[0];
        }

        return str_to_num(s);
    }

    expect(c, '(');
    x = expr_num(c);
    expect(c, ')');

    switch (code)
    {
    case F_ABS: return math_abs(x);
    case F_ATN: return math_atn(x);
    case F_COS: return math_cos(x);
    case F_EXP: return math_exp(x);
    case F_INT: return math_floor(x);
    case F_LOG: return math_log(x);
    case F_SIN: return math_sin(x);
    case F_SQR: return math_sqr(x);
    case F_TAN: return math_tan(x);
    default:    return x < 0.0f ? -1.0f : (x > 0.0f ? 1.0f : 0.0f);
    }
}

static float user_fn(Cur* c, int32_t letter)
{
    Cur body;
    float saved = local_value;
    float arg = 0.0f;
    int32_t saved_slot = local_slot;
    float result;
    int32_t slot = fn_param[letter];

    c->p++;

    if (!fn_defined[letter]) err_die(E_C09, err_line, 0);

    if (slot >= 0)
    {
        expect(c, '(');
        arg = expr_num(c);
        expect(c, ')');
    }

    local_slot = slot + 1;
    local_value = arg;
    body.p = fn_body[letter];
    result = expr_num(&body);
    if (!cur_end(&body)) err_die(E_C01, err_line, 0);

    local_slot = saved_slot;
    local_value = saved;

    return result;
}

static float* array_ref(Cur* c)
{
    int32_t letter = cur_at(c)->slot;
    int32_t i;
    int32_t j = 0;
    int32_t ndim = 1;

    c->p++;
    expect(c, '(');
    i = math_round(expr_num(c));

    if (cur_punct(c, ','))
    {
        c->p++;
        j = math_round(expr_num(c));
        ndim = 2;
    }
    expect(c, ')');

    return var_array_at(letter, i, j, ndim);
}

float* expr_ref(Cur* c)
{
    if (cur_at(c)->kind == T_ARRAY) return array_ref(c);
    if (cur_at(c)->kind != T_NUMVAR) err_die(E_C01, err_line, 0);
    return var_num_ref(tokens[c->p++].slot);
}

static float primary(Cur* c)
{
    const Token* t = cur_at(c);

    switch (t->kind)
    {
    case T_NUM:
        c->p++;
        return t->num;

    case T_NUMVAR:
        c->p++;
        return t->slot + 1 == local_slot ? local_value : var_num_get(t->slot);

    case T_ARRAY:
        return *array_ref(c);

    case T_STR: case T_STRVAR:
        err_die(E_C10, err_line, 0);
        return 0.0f;

    case T_FUNC:
        return builtin(c, t->aux);

    case T_FN:
        return user_fn(c, t->slot);

    case T_PUNCT:
        if (t->aux == '(')
        {
            float x;

            c->p++;
            x = expr_num(c);
            expect(c, ')');

            return x;
        }
        break;

    default:
        break;
    }

    err_die(E_C01, err_line, 0);

    return 0.0f;
}

static float factor(Cur* c)
{
    float x = primary(c);

    while (cur_punct(c, '^'))
    {
        c->p++;
        x = chk(math_pow(x, primary(c)));
    }

    return x;
}

static float term(Cur* c)
{
    float x = factor(c);

    while (cur_punct(c, '*') || cur_punct(c, '/'))
    {
        int32_t op = cur_at(c)->aux;
        float y;

        c->p++;
        y = factor(c);

        if (op == '/' && y == 0.0f) err_die(E_R03, err_line, 0);

        x = chk((op == '*') ? x * y : x / y);
    }

    return x;
}

float expr_num(Cur* c)
{
    float x;
    int32_t negate = 0;

    if (cur_punct(c, '+') || cur_punct(c, '-'))
    {
        negate = cur_at(c)->aux == '-';
        c->p++;
    }

    x = term(c);
    if (negate) x = -x;

    while (cur_punct(c, '+') || cur_punct(c, '-'))
    {
        int32_t op = cur_at(c)->aux;

        c->p++;
        x = chk((op == '+') ? x + term(c) : x - term(c));
    }

    return chk(x);
}

const char* expr_str(Cur* c)
{
    const Token* t = cur_at(c);

    if (t->kind == T_STR)
    {
        c->p++;
        return &strpool[t->slot];
    }
    if (t->kind == T_STRVAR)
    {
        c->p++;
        return var_str_get(t->slot);
    }

    err_die(E_C10, err_line, 0);

    return "";
}

int32_t expr_is_str(const Cur* c)
{
    return cur_at(c)->kind == T_STR || cur_at(c)->kind == T_STRVAR;
}

static int32_t is_relop(const Cur* c)
{
    const Token* t = cur_at(c);

    if (t->kind != T_PUNCT) return 0;

    return t->aux == '=' || t->aux == '<' || t->aux == '>'
           || t->aux == P_LE || t->aux == P_GE || t->aux == P_NE;
}

static int32_t starts_relation(const Cur* c)
{
    int32_t depth = 0;
    uint32_t k = c->p;

    for (;;)
    {
        const Token* t = &tokens[k];

        if (t->kind == T_EOL) return 0;
        if (t->kind == T_KW
            && (t->aux == K_AND || t->aux == K_OR || t->aux == K_THEN
                || t->aux == K_ELSE) && depth == 0)
        {
            return 0;
        }

        if (t->kind == T_PUNCT)
        {
            if (t->aux == '(') depth++;
            else if (t->aux == ')')
            {
                if (depth == 0) return 0;
                depth--;
            }
            else if (depth == 0 && t->aux == ':') return 0;
        }

        {
            Cur probe;

            probe.p = k;
            if (depth == 0 && is_relop(&probe)) return 1;
        }
        k++;
    }
}

static int32_t relation(Cur* c)
{
    int32_t op;

    if (expr_is_str(c))
    {
        const char* a = expr_str(c);
        const char* b;
        int32_t diff;

        if (!is_relop(c)) err_die(E_C01, err_line, 0);
        op = cur_at(c)->aux;
        c->p++;

        if (op != '=' && op != P_NE) err_die(E_C10, err_line, 0);
        if (!expr_is_str(c)) err_die(E_C10, err_line, 0);

        b = expr_str(c);
        diff = str_cmp(a, b);

        return op == '=' ? (diff == 0) : (diff != 0);
    }

    {
        float a = expr_num(c);
        float b;

        if (!is_relop(c)) err_die(E_C01, err_line, 0);
        op = cur_at(c)->aux;
        c->p++;

        if (expr_is_str(c)) err_die(E_C10, err_line, 0);
        b = expr_num(c);

        switch (op)
        {
        case '=':  return a == b;
        case '<':  return a < b;
        case '>':  return a > b;
        case P_LE: return a <= b;
        case P_GE: return a >= b;
        default:   return a != b;
        }
    }
}

static int32_t logical_primary(Cur* c)
{
    if (cur_punct(c, '(') && !starts_relation(c))
    {
        int32_t v;

        c->p++;
        v = expr_cond(c);
        expect(c, ')');

        return v;
    }

    return relation(c);
}

static int32_t logical_factor(Cur* c)
{
    if (cur_kw(c, K_NOT))
    {
        c->p++;

        return !logical_primary(c);
    }

    return logical_primary(c);
}

static int32_t logical_term(Cur* c)
{
    int32_t v = logical_factor(c);

    while (cur_kw(c, K_AND))
    {
        c->p++;

        v = logical_factor(c) && v;
    }

    return v;
}

int32_t expr_cond(Cur* c)
{
    int32_t v = logical_term(c);

    while (cur_kw(c, K_OR))
    {
        c->p++;
        v = logical_term(c) || v;
    }

    return v;
}
