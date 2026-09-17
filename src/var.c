#include "basic.h"

static float numvars[NUM_SLOTS];
static char strvars[STR_SLOTS][STR_BUF];

typedef struct
{
    uint16_t off;
    uint16_t d1;
    uint16_t d2;
    uint8_t ndim;
    uint8_t declared;
} Array;

static Array arrays[ARR_SLOTS];
static float pool[ARR_POOL];
static int32_t poolused;
static int32_t base;

void var_reset(void)
{
    int32_t i;
    int32_t j;

    for (i = 0; i < NUM_SLOTS; i++) numvars[i] = 0.0f;
    for (i = 0; i < STR_SLOTS; i++) strvars[i][0] = '\0';
    for (i = 0; i < ARR_SLOTS; i++)
    {
        arrays[i].declared = 0;
        arrays[i].ndim = 0;
    }
    for (j = 0; j < ARR_POOL; j++) pool[j] = 0.0f;

    poolused = 0;
    base = 0;
}

float* var_num_ref(int32_t slot)
{
    return &numvars[slot];
}

float var_num_get(int32_t slot)
{
    return numvars[slot];
}

void var_num_set(int32_t slot, float value)
{
    numvars[slot] = value;
}

const char* var_str_get(int32_t slot)
{
    return strvars[slot];
}

void var_str_set(int32_t slot, const char* value)
{
    int32_t i = 0;

    while (value[i] != '\0')
    {
        if (i >= STR_LEN) err_die(E_R10, err_line, 0);
        strvars[slot][i] = value[i];
        i++;
    }
    strvars[slot][i] = '\0';
}

void var_set_base(int32_t value)
{
    base = value;
}

int32_t var_get_base(void)
{
    return base;
}

static void allocate(int32_t letter, int32_t d1, int32_t d2, int32_t ndim)
{
    int32_t count;

    if (d1 < base || (ndim == 2 && d2 < base)) err_die(E_C01, err_line, 0);
    if (d1 >= ARR_POOL || (ndim == 2 && d2 >= ARR_POOL)) err_die(E_C15, 0, 0);
    count = (d1 - base + 1) * (ndim == 2 ? (d2 - base + 1) : 1);

    if (poolused + count > ARR_POOL) err_die(E_C15, 0, 0);

    arrays[letter].off = (uint16_t)poolused;
    arrays[letter].d1 = (uint16_t)d1;
    arrays[letter].d2 = (uint16_t)d2;
    arrays[letter].ndim = (uint8_t)ndim;
    arrays[letter].declared = 1;

    poolused += count;
}

void var_array_dim(int32_t letter, int32_t d1, int32_t d2, int32_t ndim)
{

    if (arrays[letter].declared) err_die(E_C08, err_line, 0);

    allocate(letter, d1, d2, ndim);
}

float* var_array_at(int32_t letter, int32_t i, int32_t j, int32_t ndim)
{
    Array* a = &arrays[letter];
    int32_t span;
    int32_t index;

    if (!a->declared) allocate(letter, 10, 10, ndim);

    if (ndim != a->ndim) err_die(E_R18, err_line, 0);
    if (i < base || i > a->d1) err_die(E_R01, err_line, 0);

    if (ndim == 1) return &pool[a->off + (i - base)];

    if (j < base || j > a->d2) err_die(E_R01, err_line, 0);

    span = a->d2 - base + 1;
    index = (i - base) * span + (j - base);

    return &pool[a->off + index];
}
