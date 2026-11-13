# test_overeni/b320/start.S - start PS-EXE mapa_dune.c: zasobnik, vynulovat .bss, main()
        .set noreorder
        .set noat
        .section .text.start, "ax"
        .globl _start
_start:
        lui     $sp, 0x801f
        ori     $sp, $sp, 0xfff0
        la      $t0, __bss_start
        la      $t1, __bss_end
1:      beq     $t0, $t1, 2f
        nop
        sw      $zero, 0($t0)
        b       1b
        addiu   $t0, $t0, 4
2:      jal     main
        nop
3:      b       3b
        nop
