# B317: maly program pro PlayStation (PS-EXE) - "hra", ktera cte ovladace / mys
# PRESNE jako hry: pres BIOS (InitPAD + StartPAD, B0h funkce 12h a 13h). BIOS pak
# kazdy snimek (VBlank) precte port 1 i port 2 do dvou bufferu:
#   [0] 00 = zarizeni odpovedelo, FF = v portu nic neni
#   [1] ID: 12 = mys PlayStation, 41 = standardni ovladac
#   [2] [3] tlacitka (0 = stisknuto); mys: [3] bit 3 = leve, bit 2 = prave
#   [4] [5] mys: posun X a Y od minuleho cteni (-128..127)
# Program kazdy novy buffer zpracuje (jako hra: kurzor += posun, pocita kliky)
# a vysledky necha v RAM, kde je test cte (integrace_b317.cpp):
#   0x80100100 port 1, 0x80100140 port 2 - slova:
#   +0 cteni  +4 posledni (stav << 8 | ID)  +8 soucet X  +12 soucet Y
#   +16 stisku leveho  +20 stisku praveho  +24 snimku s levym  +28 snimku s pravym
#   +32 posledni tlacitka (hi << 8 | lo)  +36 / +40 leve / prave minule (vnitrni)
#   +44 cteni s mysi (12)  +48 cteni s ovladacem (41)  +52 cteni "nic v portu" (FF)
#   +56 nejvetsi |posun X| v jednom cteni
# Procesor R3000: po kazdem nacteni z pameti jedna instrukce, ktera nacteny registr
# nepouziva (load delay), po skoku instrukce ve zpozdovacim slotu - psano rucne.
        .set noreorder
        .set noat
        .text
        .globl _start
_start:
        lui     $sp, 0x801f
        ori     $sp, $sp, 0xfff0
        lui     $s0, 0x8010             # buffer portu 1 = 0x80100000
        addiu   $s1, $s0, 0x40          # buffer portu 2 = 0x80100040
        addiu   $s2, $s0, 0x100         # vysledky = 0x80100100
        move    $t0, $s2                # vysledky vynulovat (64 slov)
        addiu   $t1, $s2, 256
1:      sw      $zero, 0($t0)
        addiu   $t0, $t0, 4
        bne     $t0, $t1, 1b
        nop
        # InitPAD(buf1, 34, buf2, 34) = B(12h)
        move    $a0, $s0
        li      $a1, 34
        move    $a2, $s1
        li      $a3, 34
        li      $t2, 0xb0
        jalr    $t2
        li      $t1, 0x12               # (zpozdovaci slot) cislo funkce B0
        # StartPAD() = B(13h)
        li      $t2, 0xb0
        jalr    $t2
        li      $t1, 0x13
        li      $t0, 0xee               # oznaceni "zpracovano" (BIOS prepise na 00 / FF)
        sb      $t0, 0($s0)
        sb      $t0, 0($s1)
hlavni:
        lbu     $t0, 0($s0)
        li      $t1, 0xee               # (load delay)
        beq     $t0, $t1, port2
        nop
        move    $a0, $s0
        jal     zpracuj
        addiu   $a1, $s2, 0             # (zpozdovaci slot) vysledky portu 1
port2:
        lbu     $t0, 0($s1)
        li      $t1, 0xee
        beq     $t0, $t1, hlavni
        nop
        move    $a0, $s1
        jal     zpracuj
        addiu   $a1, $s2, 0x40          # vysledky portu 2
        b       hlavni
        nop

# zpracuj(a0 = buffer, a1 = vysledky)
zpracuj:
        lw      $t0, 0($a1)
        nop
        addiu   $t0, $t0, 1
        sw      $t0, 0($a1)             # cteni++
        lbu     $t2, 0($a0)             # stav
        lbu     $t3, 1($a0)             # ID
        sll     $t4, $t2, 8
        or      $t4, $t4, $t3
        sw      $t4, 4($a1)
        li      $t5, 0xee
        sb      $t5, 0($a0)             # buffer zpracovan
        li      $t5, 0xff
        bne     $t2, $t5, 1f
        nop
        lw      $t0, 52($a1)            # v portu nic
        nop
        addiu   $t0, $t0, 1
        sw      $t0, 52($a1)
        jr      $ra
        nop
1:      li      $t5, 0x41
        bne     $t3, $t5, 2f
        nop
        lbu     $t6, 2($a0)             # ovladac: tlacitka
        lbu     $t7, 3($a0)
        nop
        sll     $t7, $t7, 8
        or      $t6, $t6, $t7
        sw      $t6, 32($a1)
        lw      $t0, 48($a1)
        nop
        addiu   $t0, $t0, 1
        sw      $t0, 48($a1)
        jr      $ra
        nop
2:      li      $t5, 0x12
        bne     $t3, $t5, 9f            # jine zarizeni - nic
        nop
        lw      $t0, 44($a1)            # mys
        nop
        addiu   $t0, $t0, 1
        sw      $t0, 44($a1)
        lbu     $t6, 2($a0)             # tlacitka lo
        lbu     $t7, 3($a0)             # tlacitka hi
        lb      $t8, 4($a0)             # posun X (se znamenkem)
        lb      $t9, 5($a0)             # posun Y
        sll     $v0, $t7, 8
        or      $v0, $v0, $t6
        sw      $v0, 32($a1)
        lw      $v1, 8($a1)
        nop
        addu    $v1, $v1, $t8
        sw      $v1, 8($a1)             # kurzor X += posun
        lw      $v1, 12($a1)
        nop
        addu    $v1, $v1, $t9
        sw      $v1, 12($a1)            # kurzor Y += posun
        bgez    $t8, 3f                 # |posun X|
        move    $v1, $t8
        subu    $v1, $zero, $t8
3:      lw      $v0, 56($a1)
        nop
        sltu    $t0, $v0, $v1
        beq     $t0, $zero, 4f
        nop
        sw      $v1, 56($a1)
4:      andi    $v0, $t7, 0x08          # leve tlacitko (0 = stisknuto)
        sltiu   $v0, $v0, 1
        lw      $v1, 36($a1)
        nop
        sw      $v0, 36($a1)
        beq     $v0, $zero, 5f
        nop
        lw      $t0, 24($a1)
        nop
        addiu   $t0, $t0, 1
        sw      $t0, 24($a1)
        bne     $v1, $zero, 5f
        nop
        lw      $t0, 16($a1)            # nove stisknuto
        nop
        addiu   $t0, $t0, 1
        sw      $t0, 16($a1)
5:      andi    $v0, $t7, 0x04          # prave tlacitko
        sltiu   $v0, $v0, 1
        lw      $v1, 40($a1)
        nop
        sw      $v0, 40($a1)
        beq     $v0, $zero, 9f
        nop
        lw      $t0, 28($a1)
        nop
        addiu   $t0, $t0, 1
        sw      $t0, 28($a1)
        bne     $v1, $zero, 9f
        nop
        lw      $t0, 20($a1)
        nop
        addiu   $t0, $t0, 1
        sw      $t0, 20($a1)
9:      jr      $ra
        nop
