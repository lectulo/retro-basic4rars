#include "basic.h"

static char source[SRC_MAX];

void run(const char* path)
{
    int32_t n;

    if (path == NULL)
    {
        out_puts("usage: basic <program.bas>\n");
        port_exit(2);
    }

    n = port_src_load(path, source, SRC_MAX);
    if (n == -2) err_die(E_C15, 0, 0);
    if (n < 0)
    {
        out_puts("CANNOT READ ");
        out_puts(path);
        port_putc('\n');
        port_exit(2);
    }

    lex_program(source, n);

    prog_pass0();
    exec_program();

    port_exit(0);
}
