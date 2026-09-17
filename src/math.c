#include "basic.h"

#define PI_2_HI     1.5703125f
#define PI_2_LO     0.0004838267923332751f
#define PI_2_TAIL   2.5633441515945189e-12f
#define LN2         0.6931471824645996f

#define LN2_HI      0.693115234375f
#define LN2_LO      3.194618329871446e-05f
#define PI_4        0.7853981852531433f
#define SQRT1_2     0.7071067690849304f
#define SQRT2       1.4142135381698608f
#define TAN_PI_8    0.4142135679721832f
#define EXP_MAX     88.02969360351562f

float math_abs(float x)
{
    return x < 0.0f ? -x : x;
}

float math_floor(float x)
{
    float t;

    if (math_abs(x) >= 8388608.0f) return x;

    t = (float)(int32_t)x;
    if (t > x) t -= 1.0f;

    return t;
}

int32_t math_round(float x)
{
    float t = x;
    if (math_abs(x) < 8388608.0f)
    {
        t = math_floor(math_abs(x));
        if (math_abs(x) - t >= 0.5f) t += 1.0f;
        if (x < 0.0f) t = -t;
    }

    if (t >= 2147483647.0f) return 2147483647;
    if (t <= -2147483648.0f) return -2147483647 - 1;

    return (int32_t)t;
}

float math_sqr(float x)
{
    if (x < 0.0f) err_die(E_R04, err_line, 0);

    return __builtin_sqrtf(x);
}

float math_exp(float x)
{
    int32_t k;
    float r;
    float y;

    if (x > EXP_MAX) err_die(E_R14, err_line, 0);
    if (x < -EXP_MAX) return 0.0f;

    k = math_round(x / LN2);
    r = (x - (float)k * LN2_HI) - (float)k * LN2_LO;

    y = 1.0f + r * (1.0f + r * (0.5f + r * (0.16666667f
        + r * (0.041666668f + r * (0.008333334f
        + r * (0.0013888889f + r * 0.00019841270f))))));

    while (k > 0)
    {
        y *= 2.0f;
        k--;
    }
    while (k < 0)
    {
        y *= 0.5f;
        k++;
    }

    return y;
}

float math_log(float x)
{
    int32_t k = 0;
    float s;
    float s2;

    if (x <= 0.0f) err_die(E_R04, err_line, 0);

    while (x >= SQRT2)
    {
        x *= 0.5f;
        k++;
    }
    while (x < SQRT1_2)
    {
        x *= 2.0f;
        k--;
    }

    s = (x - 1.0f) / (x + 1.0f);
    s2 = s * s;

    return (float)k * LN2
           + 2.0f * s * (1.0f + s2 * (0.33333334f
             + s2 * (0.2f + s2 * 0.14285715f)));
}

static float poly_sin(float r)
{
    float r2 = r * r;

    return r * (1.0f + r2 * (-0.16666667f
           + r2 * (0.008333334f + r2 * (-0.00019841270f
           + r2 * 0.0000027557319f))));
}

static float poly_cos(float r)
{
    float r2 = r * r;

    return 1.0f + r2 * (-0.5f + r2 * (0.041666668f
           + r2 * (-0.0013888889f + r2 * 0.000024801587f)));
}

#define TRIG_MAX    4194304.0f

static void two_prod(float a, float b, float* p, float* err);

static float sin_cos(float x, int32_t quarter)
{
    int32_t n;
    float r;
    float high;
    float p;
    float error;

    if (math_abs(x) >= TRIG_MAX) err_die(E_R17, err_line, 0);

    n = math_round(x / (PI_2_HI + PI_2_LO));

    high = (float)(n / 4096) * 4096.0f;
    r = (x - high * PI_2_HI) - ((float)n - high) * PI_2_HI;
    two_prod((float)n, PI_2_LO, &p, &error);
    r = ((r - p) - error) - (float)n * PI_2_TAIL;

    n = (n + quarter) & 3;

    if (n == 0) return poly_sin(r);
    if (n == 1) return poly_cos(r);
    if (n == 2) return -poly_sin(r);

    return -poly_cos(r);
}

float math_sin(float x)
{
    return sin_cos(x, 0);
}

float math_cos(float x)
{
    return sin_cos(x, 1);
}

float math_tan(float x)
{
    float s = math_sin(x);
    float c = math_cos(x);

    if (math_abs(c) < math_abs(s) / NUM_MAX) err_die(E_R14, err_line, 0);

    return s / c;
}

float math_atn(float x)
{
    int32_t negate = 0;
    int32_t invert = 0;
    int32_t shift = 0;
    float t2;
    float y;

    if (x < 0.0f)
    {
        x = -x;
        negate = 1;
    }
    if (x > 1.0f)
    {
        x = 1.0f / x;
        invert = 1;
    }
    if (x > TAN_PI_8)
    {
        x = (x - 1.0f) / (x + 1.0f);
        shift = 1;
    }

    t2 = x * x;
    y = x * (1.0f + t2 * (-0.33333334f + t2 * (0.2f
        + t2 * (-0.14285715f + t2 * (0.11111111f
        + t2 * (-0.09090909f + t2 * (0.07692308f
        + t2 * -0.06666667f)))))));

    if (shift) y += PI_4;
    if (invert) y = (PI_2_HI + PI_2_LO) - y;
    if (negate) y = -y;

    return y;
}

static float pow_int(float a, int32_t n)
{
    float y = 1.0f;
    int32_t negative = n < 0;

    if (negative) { n = -n; a = 1.0f / a; }

    while (n > 0)
    {
        if (n & 1) y *= a;
        a *= a;
        n >>= 1;
    }

    return y;
}

float math_pow(float a, float b)
{
    float y;

    if (a == 0.0f)
    {

        if (b < 0.0f) err_die(E_R16, err_line, 0);

        return b == 0.0f ? 1.0f : 0.0f;
    }

    if (b == math_floor(b) && math_abs(b) <= 1073741824.0f)
    {
        y = pow_int(a, (int32_t)b);
    }
    else
    {

        if (a < 0.0f && b != math_floor(b)) err_die(E_R04, err_line, 0);

        y = math_exp(b * math_log(math_abs(a)));
    }

    if (math_abs(y) > NUM_MAX) err_die(E_R14, err_line, 0);

    return y;
}

static uint32_t rnd_state = 1;

float math_rnd(void)
{
    rnd_state = rnd_state * 1103515245u + 12345u;

    return (float)(rnd_state >> 8) * (1.0f / 16777216.0f);
}

void math_randomize(void)
{
    rnd_state = port_seed() | 1u;
}

const float math_p10[11] =
{
    1e0f, 1e1f, 1e2f, 1e3f, 1e4f, 1e5f,
    1e6f, 1e7f, 1e8f, 1e9f, 1e10f
};

#define SPLITTER        4097.0f

static void split(float a, float* h, float* l)
{
    float t = a * SPLITTER;

    *h = t - (t - a);
    *l = a - *h;
}

static void two_prod(float a, float b, float* p, float* err)
{
    float ah;
    float al;
    float bh;
    float bl;

    *p = a * b;
    split(a, &ah, &al);
    split(b, &bh, &bl);
    *err = ((ah * bh - *p) + ah * bl + al * bh) + al * bl;
}

Dbl math_dbl_mul(Dbl a, float b)
{
    Dbl r;
    float p;
    float e;

    two_prod(a.hi, b, &p, &e);
    e += a.lo * b;
    r.hi = p + e;
    r.lo = e - (r.hi - p);

    return r;
}

Dbl math_dbl_div(Dbl a, float b)
{
    Dbl r;
    float q;
    float p;
    float e;
    float t;

    q = a.hi / b;
    two_prod(q, b, &p, &e);

    r.hi = q;
    r.lo = (((a.hi - p) - e) + a.lo) / b;

    t = r.hi + r.lo;
    r.lo = r.lo - (t - r.hi);
    r.hi = t;

    return r;
}
