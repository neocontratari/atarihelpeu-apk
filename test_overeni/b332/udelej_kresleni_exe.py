#!/usr/bin/env python3
"""B332: PS-EXE (spustitelny soubor PlayStation) z kresleni_test.c.

Preklad: clang (mipsel, -march=mips1 = R3000; prekladac sam hlida "load delay") -> ld.lld
(kresleni_test.ld: vse od 0x80010000) -> llvm-objcopy (holy kod) -> hlavicka "PS-X EXE"
2048 bajtu (pc0 = _start, t_addr, t_size, zasobnik; znacka oblasti Europe). Jadro PCSX ho
nahraje primo (libretro.c: soubor .exe -> Load()) s BIOSem HLE.
Pouziti: python3 udelej_kresleni_exe.py PRAC   -> PRAC/kresleni_test.exe
"""
import os
import struct
import subprocess
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
ADRESA = 0x80010000


def main():
    prac = sys.argv[1]
    os.makedirs(prac, exist_ok=True)
    o, elf, binr, exe = (os.path.join(prac, 'kresleni_test' + p) for p in ('.o', '.elf', '.bin', '.exe'))
    subprocess.run(['clang', '--target=mipsel-unknown-elf', '-march=mips1', '-mabi=32', '-fno-pic', '-mno-abicalls',
                    '-O2', '-ffreestanding', '-fno-builtin', '-G0', '-Wno-unused-command-line-argument',
                    '-c', os.path.join(ZDE, 'kresleni_test.c'), '-o', o], check=True,
                   stderr=subprocess.DEVNULL)   # jen "MIPS-I support is experimental"
    subprocess.run(['ld.lld', '-m', 'elf32ltsmip', '-T', os.path.join(ZDE, 'kresleni_test.ld'), '-static', o, '-o', elf],
                   check=True)
    subprocess.run(['llvm-objcopy', '-O', 'binary', '--only-section=.text', '--only-section=.rodata',
                    '--only-section=.data', elf, binr], check=True)
    nm = subprocess.run(['llvm-nm', elf], check=True, capture_output=True, text=True).stdout
    start = [int(r.split()[0], 16) for r in nm.splitlines() if r.split()[-1] == '_start'][0]
    kod = open(binr, 'rb').read()
    delka = len(kod)
    kod += b'\0' * (-len(kod) % 2048)
    hl = bytearray(2048)
    hl[0:8] = b'PS-X EXE'
    # pc0, gp0, t_addr, t_size, d_addr, d_size, b_addr, b_size, s_addr, s_size
    struct.pack_into('<10I', hl, 0x10, start, 0, ADRESA, len(kod), 0, 0, 0, 0, 0x801FFFF0, 0)
    znacka = b'Sony Computer Entertainment Inc. for Europe area'
    hl[0x4C:0x4C + len(znacka)] = znacka
    open(exe, 'wb').write(bytes(hl) + kod)
    print('PS-EXE %s: %d bajtu kodu na 0x%08X, start 0x%08X' % (exe, delka, ADRESA, start))


if __name__ == '__main__':
    main()
