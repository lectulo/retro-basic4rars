#include "basic.h"

static int32_t is_digit(char c)
{
    return c >= '0' && c <= '9';
}

int32_t str_len(const char* s)
{
    int32_t n = 0;

    while (s[n] != '\0') n++;

    return n;
}

int32_t str_eq(const char* a, const char* b)
{
    return str_cmp(a, b) == 0;
}

int32_t str_scan_num(const char* p, int32_t n, float* out)
{
    int32_t i = 0;
    int32_t digits = 0;
    int32_t kept = 0;
    int32_t scale = 0;
    int32_t point = 0;
    Dbl value = {0.0f, 0.0f};

    while (i < n)
    {
        if (p[i] == '.' && !point) { point = 1; i++; continue; }
        if (!is_digit(p[i])) break;
        if (point) scale--;
        digits++;
        if (kept < 14)
        {
            float hi;
            value = math_dbl_mul(value, 10.0f);
            hi = value.hi + (float)(p[i] - '0');
            value.lo += (value.hi - hi) + (float)(p[i] - '0');
            value.hi = hi;
            if (hi != 0.0f) kept++;
        }
        else scale++;
        i++;
    }
    if (!digits) return 0;

    if (i < n && (p[i] == 'E' || p[i] == 'e'))
    {
        int32_t save = i++;
        int32_t sign = 1;
        int32_t exponent = 0;
        int32_t start;
        if (i < n && (p[i] == '+' || p[i] == '-'))
        {
            if (p[i] == '-') sign = -1;
            i++;
        }
        start = i;
        while (i < n && is_digit(p[i]))
        {

            if (exponent < 1000) exponent = exponent * 10 + p[i] - '0';
            i++;
        }
        if (start == i) i = save;
        else scale += sign * exponent;
    }
    if (value.hi == 0.0f || scale < -60) { *out = 0.0f; return i; }
    if (scale + kept > 39) return -1;

    if (scale > 0)
    {
        value = math_dbl_mul(value, math_p10[scale % 10]);
        for (scale /= 10; scale > 0; scale--) value = math_dbl_mul(value, 1e10f);
    }
    else
    {
        while (scale <= -10) { value = math_dbl_div(value, 1e10f); scale += 10; }
        value = math_dbl_div(value, math_p10[-scale]);
    }
    *out = value.hi + value.lo;
    if (*out != *out || *out > NUM_MAX) return -1;
    return i;
}

float str_to_num(const char* s)
{
    int32_t n = str_len(s);
    int32_t i = 0;
    int32_t sign = 1;
    int32_t used;
    float value;

    while (i < n && s[i] == ' ') i++;
    while (n > i && s[n - 1] == ' ') n--;

    if (i < n && (s[i] == '+' || s[i] == '-'))
    {
        if (s[i] == '-') sign = -1;
        i++;
    }

    used = str_scan_num(&s[i], n - i, &value);
    if (used != n - i || used <= 0) err_die(E_R12, err_line, 0);

    return sign < 0 ? -value : value;
}

int32_t str_cmp(const char* a, const char* b)
{
    while (*a != '\0' && *a == *b)
    {
        a++;
        b++;
    }

    return (int32_t)(unsigned char)*a - (int32_t)(unsigned char)*b;
}
