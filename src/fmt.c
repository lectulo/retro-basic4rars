#include "basic.h"

static int32_t pos;

static void decompose(float a, int32_t* m, int32_t* k)
{
    Dbl v;
    int32_t e = 0;
    int32_t n;
    float frac;

    v.hi = a;
    v.lo = 0.0f;

    while (v.hi >= 1e10f)
    {
        v = math_dbl_div(v, 1e10f);
        e += 10;
    }
    while (v.hi < 1e-10f)
    {
        v = math_dbl_mul(v, 1e10f);
        e -= 10;
    }

    if (v.hi >= 1.0f)
    {
        for (n = 10; n > 0; n--)
        {
            if (v.hi >= math_p10[n]) break;
        }
        v = math_dbl_div(v, math_p10[n]);
        e += n;
    }
    else
    {
        for (n = 1; n <= 10; n++)
        {
            if (v.hi * math_p10[n] >= 1.0f) break;
        }
        v = math_dbl_mul(v, math_p10[n]);
        e -= n;
    }

    v = math_dbl_mul(v, 1e5f);

    n = (int32_t)v.hi;
    frac = (v.hi - (float)n) + v.lo;
    if (frac >= 0.5f) n++;

    if (n >= 1000000)
    {
        n /= 10;
        e++;
    }

    *m = n;
    *k = e;
}

static int32_t digits_of(int32_t m, char* d)
{
    int32_t i;
    int32_t nsig = 6;

    for (i = 5; i >= 0; i--)
    {
        d[i] = (char)('0' + m % 10);
        m /= 10;
    }
    while (nsig > 1 && d[nsig - 1] == '0') nsig--;

    return nsig;
}

int32_t fmt_num(float x, char* buf)
{
    char d[6];
    int32_t m;
    int32_t k;
    int32_t nsig;
    int32_t total;
    int32_t i;
    int32_t n = 0;
    float a;

    buf[n++] = (x < 0.0f) ? '-' : ' ';
    a = math_abs(x);

    if (a == 0.0f)
    {
        buf[n++] = '0';
        buf[n++] = ' ';
        return n;
    }

    decompose(a, &m, &k);
    nsig = digits_of(m, d);

    if (k <= 5 && a == (float)(int32_t)a)
    {
        for (i = 0; i <= k; i++) buf[n++] = d[i];
        buf[n++] = ' ';
        return n;
    }

    total = (k >= 0) ? ((nsig > k + 1) ? nsig : k + 1) : (-k - 1 + nsig);

    if (total <= SIG_WIDTH)
    {
        if (k >= 0)
        {

            for (i = 0; i <= k; i++) buf[n++] = d[i];
            buf[n++] = '.';
            for (i = k + 1; i < nsig; i++) buf[n++] = d[i];
        }
        else
        {

            buf[n++] = '.';
            for (i = 0; i < -k - 1; i++) buf[n++] = '0';
            for (i = 0; i < nsig; i++) buf[n++] = d[i];
        }

        buf[n++] = ' ';
        return n;
    }

    buf[n++] = d[0];
    buf[n++] = '.';
    for (i = 1; i < nsig; i++) buf[n++] = d[i];

    buf[n++] = 'E';
    buf[n++] = (k < 0) ? '-' : '+';

    i = (k < 0) ? -k : k;
    if (i >= 10) buf[n++] = (char)('0' + i / 10);
    buf[n++] = (char)('0' + i % 10);

    buf[n++] = ' ';
    return n;
}

void fmt_nl(void)
{
    port_putc('\n');
    pos = 0;
}

static void put(char c)
{
    if (pos >= PRINT_MARGIN) fmt_nl();
    port_putc(c);
    pos++;
}

void fmt_item(const char* s)
{
    int32_t len = str_len(s);
    int32_t i;

    if (pos > 0 && pos + len > PRINT_MARGIN) fmt_nl();

    for (i = 0; i < len; i++) put(s[i]);
}

void fmt_comma(void)
{
    int32_t next = (pos / PRINT_ZONE + 1) * PRINT_ZONE;

    if (next > (PRINT_ZONES - 1) * PRINT_ZONE)
    {
        fmt_nl();
        return;
    }

    while (pos < next) put(' ');
}

void fmt_tab(float x)
{
    int32_t n;

    if (x >= 2147483648.0f)
    {
        float width = PRINT_MARGIN;
        while (width <= x * 0.5f) width *= 2.0f;
        while (width >= PRINT_MARGIN)
        {
            if (x >= width) x -= width;
            width *= 0.5f;
        }
        if (x == 0.0f) x = PRINT_MARGIN;
    }
    n = math_round(x);

    if (n < 1) err_die(E_R15, err_line, 0);

    if (n > PRINT_MARGIN) n = n - PRINT_MARGIN * ((n - 1) / PRINT_MARGIN);

    if (pos > n - 1) fmt_nl();
    while (pos < n - 1) put(' ');
}

void fmt_spc(float x)
{
    int32_t n = math_round(x);
    int32_t i;

    for (i = 0; i < n; i++) put(' ');
}
