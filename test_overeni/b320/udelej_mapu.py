#!/usr/bin/env python3
"""B320: PS-EXE mapa_dune.exe z mapa_dune.c + start.S (jako ../b318/udelej_exe.py, ale C a vic sekci).

clang (mipsel, -march=mips1 = R3000: clang vklada NOP po nacteni z pameti - load delay) -> ld.lld
(mapa_dune.ld, 0x80010000) -> llvm-objcopy (kod + konstanty + data) -> hlavicka "PS-X EXE".
Pouziti: python3 udelej_mapu.py PRAC   -> PRAC/mapa_dune.exe (+ PRAC/mapa_dune.s = vypis instrukci)
"""
import os
import struct
import subprocess
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
ADRESA = 0x80010000
CF = ['clang', '--target=mipsel-unknown-elf', '-march=mips1', '-mabi=32', '-fno-pic', '-mno-abicalls', '-G0',
      '-ffreestanding', '-fno-builtin', '-nostdlib', '-O2', '-Wno-unused-command-line-argument']


def main():
    prac = sys.argv[1]
    os.makedirs(prac, exist_ok=True)
    o1, o2, elf, binr, exe = (os.path.join(prac, p) for p in ('mapa_dune.o', 'start.o', 'mapa_dune.elf', 'mapa_dune.bin', 'mapa_dune.exe'))
    subprocess.run(CF + ['-c', os.path.join(ZDE, 'mapa_dune.c'), '-o', o1], check=True, stderr=subprocess.DEVNULL)
    subprocess.run(CF + ['-c', os.path.join(ZDE, 'start.S'), '-o', o2], check=True)
    subprocess.run(['ld.lld', '-m', 'elf32ltsmip', '-T', os.path.join(ZDE, 'mapa_dune.ld'), '-static', o2, o1, '-o', elf], check=True)
    subprocess.run(['llvm-objcopy', '-O', 'binary', '--remove-section=.bss', elf, binr], check=True)
    with open(os.path.join(prac, 'mapa_dune.s'), 'w') as f:
        subprocess.run(['llvm-objdump', '-d', '--no-show-raw-insn', elf], check=True, stdout=f)
    kod = open(binr, 'rb').read()
    n = len(kod)
    kod += b'\0' * (-len(kod) % 2048)
    sym = subprocess.run(['llvm-nm', elf], check=True, capture_output=True, text=True).stdout
    adr = {l.split()[2]: int(l.split()[0], 16) for l in sym.splitlines() if len(l.split()) == 3}
    hl = bytearray(2048)
    hl[0:8] = b'PS-X EXE'
    # pc0, gp0, t_addr, t_size, d_addr, d_size, b_addr, b_size, s_addr, s_size
    struct.pack_into('<10I', hl, 0x10, adr['_start'], 0, ADRESA, len(kod), 0, 0, 0, 0, 0x801FFFF0, 0)
    znacka = b'Sony Computer Entertainment Inc. for North America area'
    hl[0x4C:0x4C + len(znacka)] = znacka
    open(exe, 'wb').write(bytes(hl) + kod)
    print('PS-EXE %s: %d bajtu (kod + data) na 0x%08X, .bss 0x%08X-0x%08X' % (exe, n, ADRESA, adr['__bss_start'], adr['__bss_end']))


if __name__ == '__main__':
    main()
