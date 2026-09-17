#include "basic.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void port_putc(char c)
{
    putchar(c);
}

void port_gets(char* buf, int32_t size)
{
    int32_t max = size - 1;
    int32_t n = 0;
    int c;

    if (max < 0) max = 0;

    while ((c = getchar()) != EOF && c != '\n')
    {
        if (n < max) buf[n++] = (char)c;
    }

    if (n < max) buf[n++] = '\n';
    buf[n] = '\0';
}

int32_t port_src_load(const char* path, char* buf, int32_t size)
{
    FILE* f = fopen(path, "rb");
    size_t n;
    int32_t overflow;

    if (f == NULL) return -1;

    n = fread(buf, 1, (size_t)size, f);
    overflow = (n == (size_t)size && fgetc(f) != EOF);
    if (ferror(f)) { fclose(f); return -1; }
    fclose(f);

    if (overflow) return -2;

    return (int32_t)n;
}

uint32_t port_seed(void)
{
    return (uint32_t)time(NULL);
}

void port_exit(int32_t code)
{
    exit(code);
}

int main(int argc, char** argv)
{
    run(argc == 2 ? argv[1] : NULL);
    return 0;
}
