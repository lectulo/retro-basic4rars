.globl  main
        .globl  port_putc
        .globl  port_gets
        .globl  port_src_load
        .globl  port_exit
        .globl  port_seed

        .data

tailbyte:
        .space  1

        .text

main:
        beqz    a0, main_noarg
        lw      a0, 0(a1)
        j       main_go
main_noarg:
        li      a0, 0
main_go:
        call    run

port_putc:
        li      a7, 11
        ecall
        ret

port_gets:
        li      a7, 8
        ecall
        ret

port_src_load:
        mv      t0, a1
        mv      t1, a2
        li      a1, 0
        li      a7, 1024
        ecall
        bltz    a0, load_fail
        mv      t2, a0

        mv      a0, t2
        mv      a1, t0
        mv      a2, t1
        li      a7, 63
        ecall
        mv      t0, a0
        bltz    t0, load_fail_close

        bne     t0, t1, load_done
        mv      a0, t2
        la      a1, tailbyte
        li      a2, 1
        li      a7, 63
        ecall
        bltz    a0, load_fail_close
        beqz    a0, load_done
        li      t0, -2

load_done:
        mv      a0, t2
        li      a7, 57
        ecall
        mv      a0, t0
        ret

load_fail_close:
        mv      a0, t2
        li      a7, 57
        ecall
load_fail:
        li      a0, -1
        ret

port_seed:
        li      a7, 30
        ecall
        ret

port_exit:
        li      a7, 93
        ecall
