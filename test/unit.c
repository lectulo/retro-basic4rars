#include "basic.h"

static int32_t failed;

static void check(int32_t ok, const char* name)
{
    if (ok) return;
    failed++;
    out_puts("FAIL: ");
    out_puts(name);
    port_putc('\n');
}

void run(const char* path)
{
    (void)path;
    check(str_len("BASIC") == 5, "string length");
    check(str_eq("ABC", "ABC") && !str_eq("ABC", "AB"), "string equality");
    check(str_cmp("AB", "AC") < 0, "string comparison");
    check(math_round(0.4999999701976776f) == 0, "round below half");
    check(math_round(1.5f) == 2 && math_round(-1.5f) == -2, "round halves");
    check(math_abs(math_sin(1000000.0f) + 0.3499935f) < 0.000001f, "sine");
    check(math_abs(math_cos(1000000.0f) - 0.9367521f) < 0.000001f, "cosine");
    check(math_abs(math_sqr(9.0f) - 3.0f) < 0.000001f, "square root");
    out_puts(failed ? "FAILED\n" : "OK\n");
    port_exit(failed ? 1 : 0);
}
