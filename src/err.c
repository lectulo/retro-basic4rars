#include "basic.h"

static const char* messages[E_COUNT] =
{
    "SYNTAX ERROR IN LINE $",
    "LINE NUMBER OUT OF RANGE",
    "DUPLICATE LINE NUMBER $",
    "UNDEFINED LINE NUMBER $ REFERENCED IN LINE $",
    "PROGRAM DOES NOT END WITH END",
    "END IS NOT THE LAST STATEMENT",
    "ARRAY AND SIMPLE VARIABLE SHARE NAME",
    "ARRAY REDIMENSIONED IN LINE $",
    "FUNCTION USED BEFORE DEFINITION IN LINE $",
    "TYPE MISMATCH IN LINE $",
    "OPTION BASE MISPLACED",
    "LINE TOO LONG",
    "ELSE WITHOUT IF IN LINE $",
    "LINE NUMBERS NOT IN ASCENDING ORDER AT LINE $",
    "PROGRAM TOO LARGE",
    "NUMERIC CONSTANT OUT OF RANGE IN LINE $",

    "SUBSCRIPT OUT OF RANGE IN LINE $",
    "OUT OF DATA IN LINE $",
    "DIVISION BY ZERO IN LINE $",
    "NEGATIVE ARGUMENT IN LINE $",
    "NEXT IN LINE $ DOES NOT MATCH FOR IN LINE $",
    "NEXT WITHOUT FOR IN LINE $",
    "RETURN WITHOUT GOSUB IN LINE $",
    "ON INDEX OUT OF RANGE IN LINE $",
    "GOSUB NESTING TOO DEEP IN LINE $",
    "STRING TOO LONG IN LINE $",
    "BAD NUMBER FORMAT IN VAL IN LINE $",
    "EMPTY STRING IN ASC IN LINE $",
    "NUMERIC OVERFLOW IN LINE $",
    "TAB ARGUMENT LESS THAN ONE IN LINE $",
    "ZERO TO NEGATIVE POWER IN LINE $",
    "ARGUMENT TOO LARGE IN LINE $",
    "WRONG NUMBER OF SUBSCRIPTS IN LINE $"
};

int32_t err_line;

void out_puts(const char* s)
{
    while (*s) port_putc(*s++);
}

void out_putn(int32_t n)
{
    char buf[12];
    int32_t i = 0;
    uint32_t value = (uint32_t)n;

    if (n < 0)
    {
        port_putc('-');
        value = 0u - value;
    }
    do
    {
        buf[i++] = (char)('0' + value % 10);
        value /= 10;
    }
    while (value > 0);

    while (i > 0) port_putc(buf[--i]);
}

const char* err_message(int32_t code)
{
    return (code < 0 || code >= E_COUNT) ? "INTERNAL ERROR" : messages[code];
}

void err_die(int32_t code, int32_t a, int32_t b)
{
    const char* m = err_message(code);
    int32_t used = 0;

    while (*m)
    {
        if (*m == '$') out_putn(used++ == 0 ? a : b);
        else port_putc(*m);
        m++;
    }
    port_putc('\n');
    port_exit(1);
}
