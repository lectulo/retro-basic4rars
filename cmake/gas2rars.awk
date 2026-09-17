function die(msg)
{
    printf("gas2rars: %s:%d: %s\n    %s\n", FILENAME, FNR, msg, $0) > "/dev/stderr"
    exit 1
}

function log2(v,    e)
{
    e = 0
    while (v > 1)
    {
        v = v / 2
        e++
    }
    return e
}

function mark(action, list,    cnt, idx, name)
{
    cnt = split(list, name, " ")
    for (idx = 1; idx <= cnt; idx++) act[name[idx]] = action
}

BEGIN {
    segment = "text"

    mark("pass", ".word .dword .half .byte .space .float .double " \
                 ".ascii .asciz .string .globl .global .extern " \
                 ".eqv .macro .end_macro .include")

    mark("text",    ".text")
    mark("data",    ".data")
    mark("section", ".section")
    mark("align",   ".align")

    mark("data",  ".bss")
    mark("align", ".p2align")
    mark("space", ".zero")
    mark("comm",  ".comm")

    mark("drop", ".file .attribute .type .size .local")
}

function sym_of(e)
{
    sub(/[+-][0-9]+$/, "", e)
    return e
}

function off_of(e)
{
    if (match(e, /[+-][0-9]+$/)) return substr(e, RSTART, RLENGTH)
    return "0"
}

function paired(reg, expr)
{
    if (reg in hi && hi[reg] != expr)
        die("%lo(" expr ") при " reg " = %hi(" hi[reg] "): пара разошлась")
}

function lo_ok(op)
{
    return op == "lw" || op == "lh" || op == "lb" || op == "flw" || op == "fld"
}

function relocate(op, operands, line,    body, rd, rs, base, expr, off)
{
    body = operands
    sub(/[ \t]*#.*$/, "", body)

    if (body ~ /%hi\(/)
    {
        if (op != "lui") die("%hi вне lui: " op)
        if (!match(body, /^[^,]+,[ \t]*%hi\(.*\)$/)) die("не разобрать %hi")

        rd = body
        sub(/[ \t]*,.*$/, "", rd)
        expr = body
        sub(/^[^,]+,[ \t]*%hi\(/, "", expr)
        sub(/\)$/, "", expr)

        hi[rd] = expr

        if (off_of(expr) == "0") return line
        return sprintf("\tlui\t%s, %%hi(%s)", rd, sym_of(expr))
    }

    if (match(body, /^[^,]+,[ \t]*[^,]+,[ \t]*%lo\(.*\)$/))
    {
        rd = body
        sub(/[ \t]*,.*$/, "", rd)
        rs = body
        sub(/^[^,]+,[ \t]*/, "", rs)
        sub(/[ \t]*,.*$/, "", rs)
        expr = body
        sub(/^.*%lo\(/, "", expr)
        sub(/\)$/, "", expr)

        paired(rs, expr)
        off = off_of(expr)
        if (off == "0") return line
        return sprintf("\taddi\t%s, %s, %%lo(%s)\n\taddi\t%s, %s, %s",
                       rd, rs, sym_of(expr), rd, rd, off)
    }

    if (match(body, /^[^,]+,[ \t]*%lo\(.*\)\([a-z0-9]+\)$/))
    {
        rd = body
        sub(/[ \t]*,.*$/, "", rd)
        base = body
        sub(/^.*\(/, "", base)
        sub(/\)$/, "", base)
        expr = body
        sub(/^[^,]+,[ \t]*%lo\(/, "", expr)
        sub(/\)\([a-z0-9]+\)$/, "", expr)

        paired(base, expr)
        off = off_of(expr)
        if (off == "0" && lo_ok(op)) return line

        return sprintf("\taddi\tt6, %s, %%lo(%s)\n\t%s\t%s, %s(t6)",
                       base, sym_of(expr), op, rd, off)
    }

    die("не разобрать %lo в " op)
}

{
    stripped = $0
    sub(/^[ \t]+/, "", stripped)

    if (stripped == "" || substr(stripped, 1, 1) == "#")
    {
        print
        next
    }

    match(stripped, /^[^ \t]+/)
    tok = substr(stripped, RSTART, RLENGTH)
    args = substr(stripped, RSTART + RLENGTH)
    sub(/^[ \t]+/, "", args)

    if (substr(tok, 1, 1) != ".")
    {

        if (stripped ~ /^[A-Za-z_.$][A-Za-z_0-9.$]*[ \t]*=[^=]/)
            die("присваивание символа: слияние глобалей не отключилось")

        if (args ~ /%hi\(|%lo\(/)
        {
            print relocate(tok, args, $0)
            next
        }

        delete hi
        print
        next
    }

    if (tok ~ /:$/)
    {
        delete hi
        print
        next
    }

    action = act[tok]
    if (action == "") die("неизвестная директива " tok)

    if (action == "pass") { print; next }
    if (action == "drop") { next }

    if (action == "text") { segment = "text"; print "\t.text"; next }
    if (action == "data") { segment = "data"; print "\t.data"; next }

    if (action == "section")
    {
        if (args ~ /\.note\.GNU-stack/) next
        if (args ~ /^\.text/) { segment = "text"; print "\t.text"; next }
        segment = "data"
        print "\t.data"
        next
    }

    if (action == "align")
    {
        if (segment != "data") next
        sub(/[ \t]*,.*$/, "", args)
        printf("\t.align\t%s\n", args)
        next
    }

    if (action == "space")
    {
        printf("\t.space\t%s\n", args)
        next
    }

    if (action == "comm")
    {
        n = split(args, a, /[ \t]*,[ \t]*/)
        if (n < 2) die("не разобрать .comm")
        segment = "data"
        print "\t.data"
        if (n >= 3) printf("\t.align\t%d\n", log2(a[3]))
        printf("%s:\n", a[1])
        printf("\t.space\t%s\n", a[2])
        next
    }

    die("действие " action " не реализовано")
}
