#pragma once

#include <stdint.h>
#include <stddef.h>

#define SRC_MAX         32768
#define MAX_LINE_LEN    72
#define LINE_BUF        80

#define MAX_LINES       512
#define MAX_TOKENS      8192
#define MAX_DATA        512

#define NUM_SLOTS       286
#define STR_SLOTS       26
#define ARR_SLOTS       26
#define ARR_POOL        4096

#define STR_LEN         18
#define STR_BUF         19

#define GOSUB_DEPTH     32
#define FOR_DEPTH       16

#define NUM_MAX         0x1p127f

#define PRINT_MARGIN    72
#define PRINT_ZONE      14
#define PRINT_ZONES     5
#define SIG_WIDTH       6
#define EXRAD_WIDTH     2

#define STR_POOL        8192

int32_t str_len(const char* s);
int32_t str_eq(const char* a, const char* b);
int32_t str_cmp(const char* a, const char* b);

int32_t str_scan_num(const char* p, int32_t n, float* out);

float str_to_num(const char* s);

enum
{
    T_EOL = 0,
    T_KW,
    T_FUNC,
    T_FN,
    T_NUM,
    T_STR,
    T_NUMVAR,
    T_STRVAR,
    T_ARRAY,
    T_PUNCT,
    T_BAD
};

enum
{
    K_AND = 0, K_BASE, K_DATA, K_DEF, K_DIM, K_ELSE, K_END, K_FOR,
    K_GO, K_GOSUB, K_GOTO, K_IF, K_INPUT, K_LET, K_NEXT, K_NOT,
    K_ON, K_OPTION, K_OR, K_PRINT, K_RANDOMIZE, K_READ, K_REM,
    K_RESTORE, K_RETURN, K_SPC, K_STEP, K_STOP, K_SUB, K_TAB,
    K_THEN, K_TO, K_COUNT
};

enum
{
    F_ABS = 0, F_ASC, F_ATN, F_COS, F_EXP, F_INT, F_LEN, F_LOG,
    F_RND, F_SGN, F_SIN, F_SQR, F_TAN, F_VAL, F_COUNT
};

enum
{
    P_LE = 1,
    P_GE,
    P_NE
};

typedef struct
{
    uint8_t  kind;
    uint8_t  aux;
    uint16_t slot;
    float    num;
} Token;

typedef struct
{
    uint16_t number;
    uint16_t start;
} Line;

extern Token   tokens[MAX_TOKENS];
extern int32_t ntokens;
extern Line    lines[MAX_LINES];
extern int32_t nlines;
extern char    strpool[STR_POOL];

void lex_program(const char* src, int32_t len);

void lex_dump(void);

typedef struct
{
    uint32_t p;
} Cur;

static inline const Token* cur_at(const Cur* c)
{
    return &tokens[c->p];
}

static inline int32_t cur_punct(const Cur* c, int32_t ch)
{
    return cur_at(c)->kind == T_PUNCT && cur_at(c)->aux == ch;
}

static inline int32_t cur_kw(const Cur* c, int32_t code)
{
    return cur_at(c)->kind == T_KW && cur_at(c)->aux == code;
}

static inline int32_t cur_end(const Cur* c)
{
    return cur_at(c)->kind == T_EOL || cur_punct(c, ':') || cur_kw(c, K_ELSE);
}

void var_reset(void);

float var_num_get(int32_t slot);
float* var_num_ref(int32_t slot);
void var_num_set(int32_t slot, float value);

const char* var_str_get(int32_t slot);
void var_str_set(int32_t slot, const char* value);

void var_set_base(int32_t base);
int32_t var_get_base(void);

void var_array_dim(int32_t letter, int32_t d1, int32_t d2, int32_t ndim);

float* var_array_at(int32_t letter, int32_t i, int32_t j, int32_t ndim);

void prog_pass0(void);
void exec_program(void);

int32_t prog_line_index(int32_t number);

void prog_data_restore(void);
const Token* prog_data_next(void);

float expr_num(Cur* c);
float* expr_ref(Cur* c);
const char* expr_str(Cur* c);
int32_t expr_cond(Cur* c);

int32_t expr_is_str(const Cur* c);

void expr_def(int32_t letter, int32_t param_slot, uint32_t body);
void expr_def_reset(void);
int32_t expr_def_exists(int32_t letter);

typedef struct
{
    float hi;
    float lo;
} Dbl;

extern const float math_p10[11];
Dbl math_dbl_mul(Dbl a, float b);
Dbl math_dbl_div(Dbl a, float b);

float math_abs(float x);
float math_floor(float x);
int32_t math_round(float x);
float math_sqr(float x);
float math_exp(float x);
float math_log(float x);
float math_sin(float x);
float math_cos(float x);
float math_tan(float x);
float math_atn(float x);
float math_pow(float a, float b);
float math_rnd(void);
void math_randomize(void);

void run(const char* path);

void port_putc(char c);

void port_gets(char* buf, int32_t size);

int32_t port_src_load(const char* path, char* buf, int32_t size);

void port_exit(int32_t code);

uint32_t port_seed(void);

#define FMT_BUF         16

int32_t fmt_num(float x, char* buf);

void fmt_item(const char* s);

void fmt_comma(void);

void fmt_tab(float x);
void fmt_spc(float x);

void fmt_nl(void);

enum
{
    E_C01 = 0, E_C02, E_C03, E_C04, E_C05, E_C06, E_C07,
    E_C08, E_C09, E_C10, E_C11, E_C12, E_C13, E_C14, E_C15, E_C16,
    E_R01, E_R02, E_R03, E_R04, E_R05, E_R06, E_R07, E_R08,
    E_R09, E_R10, E_R12, E_R13, E_R14, E_R15, E_R16, E_R17, E_R18,
    E_COUNT
};

void out_puts(const char* s);
void out_putn(int32_t n);

const char* err_message(int32_t code);

extern int32_t err_line;

void err_die(int32_t code, int32_t a, int32_t b);
