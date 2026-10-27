#!/usr/bin/env python3
"""B317 / B318: PS-EXE (spustitelny soubor PlayStation) z mys_test.S.

Preklad: clang (mipsel, R3000 = mips1) -> ld.lld (adresa 0x80010000) -> llvm-objcopy (holy kod)
-> hlavicka "PS-X EXE" 2048 bajtu (pc0, t_addr, t_size, zasobnik; znacka oblasti Europe).
Jadro PCSX ho nahraje primo (libretro.c: soubor .exe -> Load()) s BIOSem HLE.
Pouziti: python3 udelej_exe.py PRAC   -> PRAC/mys_test.exe
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
    o, elf, binr, exe = (os.path.join(prac, 'mys_test' + p) for p in ('.o', '.elf', '.bin', '.exe'))
    subprocess.run(['clang', '--target=mipsel-unknown-elf', '-march=mips1', '-mabi=32', '-fno-pic', '-mno-abicalls',
                    '-c', os.path.join(ZDE, 'mys_test.S'), '-o', o], check=True)
    subprocess.run(['ld.lld', '-m', 'elf32ltsmip', '-Ttext=0x%08x' % ADRESA, '-e', '_start', '-static', o, '-o', elf], check=True)
    subprocess.run(['llvm-objcopy', '-O', 'binary', '--only-section=.text', elf, binr], check=True)
    kod = open(binr, 'rb').read()
    kod += b'\0' * (-len(kod) % 2048)
    hl = bytearray(2048)
    hl[0:8] = b'PS-X EXE'
    # pc0, gp0, t_addr, t_size, d_addr, d_size, b_addr, b_size, s_addr, s_size
    struct.pack_into('<10I', hl, 0x10, ADRESA, 0, ADRESA, len(kod), 0, 0, 0, 0, 0x801FFFF0, 0)
    znacka = b'Sony Computer Entertainment Inc. for Europe area'
    hl[0x4C:0x4C + len(znacka)] = znacka
    open(exe, 'wb').write(bytes(hl) + kod)
    print('PS-EXE %s: %d bajtu kodu na 0x%08X' % (exe, len(open(binr, 'rb').read()), ADRESA))


if __name__ == '__main__':
    main()
