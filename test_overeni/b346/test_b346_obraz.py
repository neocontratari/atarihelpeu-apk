#!/usr/bin/env python3
"""B346: POSKOZENY OBRAZ DISKU PS1 - SKUTECNY nap_ps1_native.cpp (+ nap_ps1_obraz.h) v knihovne z ../b308/ps1_harness.cpp.
Jadro je v harnessu nahrazene: hru odmitne, kdyz cesta obsahuje "ODMITNOUT" (jinak ji spusti).

  O1) dobry .cue + .bin (cele sektory 2352 B) -> hra bezi, zadne varovani
  O2) .bin jako Command & Conquer z logu (o 143 B kratsi nez cely pocet sektoru) -> v deniku POZOR OBRAZ DISKU
      s presnymi cisly; jadro ji v harnessu prijme -> hra bezi (appka nic neblokuje)
  O3) jadro takovou hru odmitne -> vysledek "PS1_HRA_FAIL obraz" + denik "OBRAZ DISKU JE POSKOZENY"
  O4) jadro odmitne hru s DOBRYM obrazem -> porad "PS1_HRA_FAIL jadro" (beze zmeny)
  O5) vic stop: datova MODE2/2352 v poradku, hudba AUDIO s vadou -> varovani jen u hudby
  O6) MODE1/2048 a MODE2/2336 s celymi sektory -> bez varovani
  O7) .bin bez .cue: ani 2352, ani 2048 -> varovani; .iso po 2048 B -> bez varovani
  O8) dalsi start s dobrym obrazem po vadnem -> vada se nepreneze (PS1_HRA_FAIL jadro, ne obraz)
  O9) start bez disku (BIOS) - beze zmeny
Pouziti (z test_overeni/b346): EGL_PLATFORM=surfaceless python3 test_b346_obraz.py KNIHOVNA.so PRAC
  knihovna: viz JAK_SPUSTIT.txt (g++ ... ../b308/ps1_harness.cpp)   -> VYSTUP_test_b346_obraz.txt
"""
import ctypes
import os
import re
import shutil
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
LIB = ctypes.CDLL(sys.argv[1])
PRAC = os.path.abspath(sys.argv[2])
for f, typ, arg in (('h_start', ctypes.c_char_p, [ctypes.c_char_p, ctypes.c_char_p]),
                    ('h_stop', ctypes.c_char_p, []),
                    ('h_denik', ctypes.c_char_p, [])):
    fn = getattr(LIB, f)
    fn.restype = typ
    fn.argtypes = arg

vystup, chyb = [], 0


def over(ok, co):
    global chyb
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)
    if not ok:
        chyb += 1


shutil.rmtree(PRAC, ignore_errors=True)
HRY = os.path.join(PRAC, 'hry')
os.makedirs(HRY)


def soubor(jm, bajtu):
    with open(os.path.join(HRY, jm), 'wb') as f:
        f.truncate(bajtu)                      # ridky soubor - velikost je to, o co jde


def cue(jm, radky):
    with open(os.path.join(HRY, jm), 'w') as f:
        f.write('\n'.join(radky) + '\n')
    return os.path.join(HRY, jm)


def start(cesta):
    r = LIB.h_start(PRAC.encode(), cesta.encode()).decode()
    d = LIB.h_denik().decode('utf-8', 'replace')
    i = d.rfind('==================== PS1 START')   # jen posledni start
    LIB.h_stop()
    return r, d[i:]


def varovani(d):
    return [r for r in d.splitlines() if 'POZOR OBRAZ DISKU' in r]


S = 2352
soubor('dobra.bin', S * 100)
soubor('cac.bin', 557574385)                    # presne jako C&C v logu telefonu 18. 11.
soubor('ODMITNOUT_cac.bin', 557574385)
soubor('ODMITNOUT_dobra.bin', S * 100)
soubor('data.bin', S * 3000)
soubor('hudba.bin', S * 500 + 1000)
soubor('m1.bin', 2048 * 777)
soubor('m2.bin', 2336 * 555)
soubor('sama.bin', S * 10 + 7)
soubor('hra.iso', 2048 * 4000)

r, d = start(cue('dobra.cue', ['FILE "dobra.bin" BINARY', '  TRACK 01 MODE2/2352', '    INDEX 01 00:00:00']))
over(r.startswith('PS1_HRA_OK') and not varovani(d) and 'VSECHNY NALEZENY' in d, 'O1) dobry obraz: %s, bez varovani' % r)

r, d = start(cue('cac.cue', ['FILE "cac.bin" BINARY', '  TRACK 01 MODE2/2352', '    INDEX 01 00:00:00']))
v = varovani(d)
over(r.startswith('PS1_HRA_OK') and len(v) == 1
     and 'cac.bin ma 557574385 B - neni cely pocet sektoru 2352 B (237063 celych + 2209 B, do celeho chybi 143 B)' in v[0],
     'O2) obraz jako C&C: hra bezi (%s), denik: %s' % (r, v[0].split('  ', 1)[-1] if v else '-'))

r, d = start(cue('ODMITNOUT_cac.cue', ['FILE "ODMITNOUT_cac.bin" BINARY', '  TRACK 01 MODE2/2352', '    INDEX 01 00:00:00']))
over(r == 'PS1_HRA_FAIL obraz' and 'CHYBA STARTU: OBRAZ DISKU JE POSKOZENY - ODMITNOUT_cac.bin ma 557574385 B' in d,
     'O3) jadro odmitne vadny obraz -> "%s" + denik OBRAZ DISKU JE POSKOZENY' % r)

r, d = start(cue('ODMITNOUT_dobra.cue', ['FILE "ODMITNOUT_dobra.bin" BINARY', '  TRACK 01 MODE2/2352', '    INDEX 01 00:00:00']))
over(r == 'PS1_HRA_FAIL jadro' and not varovani(d) and 'OBRAZ DISKU JE POSKOZENY' not in d,
     'O4) jadro odmitne dobry obraz -> "%s" (beze zmeny)' % r)

r, d = start(cue('vic.cue', ['FILE "data.bin" BINARY', '  TRACK 01 MODE2/2352', '    INDEX 01 00:00:00',
                             'FILE "hudba.bin" BINARY', '  TRACK 02 AUDIO', '    INDEX 00 00:00:00', '    INDEX 01 00:02:00']))
v = varovani(d)
over(r.startswith('PS1_HRA_OK') and len(v) == 1 and 'hudba.bin ma 1177000 B' in v[0] and 'data.bin' not in v[0],
     'O5) vic stop: varovani jen u hudby (%s)' % (v[0].split('POZOR OBRAZ DISKU: ', 1)[-1][:70] if v else '-'))

r1, d1 = start(cue('m1.cue', ['FILE "m1.bin" BINARY', '  TRACK 01 MODE1/2048', '    INDEX 01 00:00:00']))
r2, d2 = start(cue('m2.cue', ['FILE "m2.bin" BINARY', '  TRACK 01 MODE2/2336', '    INDEX 01 00:00:00']))
over(r1.startswith('PS1_HRA_OK') and r2.startswith('PS1_HRA_OK') and not varovani(d1) and not varovani(d2),
     'O6) MODE1/2048 a MODE2/2336 s celymi sektory: bez varovani')

r1, d1 = start(os.path.join(HRY, 'sama.bin'))
r2, d2 = start(os.path.join(HRY, 'hra.iso'))
v = varovani(d1)
over(len(v) == 1 and 'sama.bin ma 23527 B' in v[0] and not varovani(d2),
     'O7) .bin bez .cue (ani 2352, ani 2048) -> varovani; .iso po 2048 B bez varovani')

start(cue('ODMITNOUT_cac2.cue', ['FILE "ODMITNOUT_cac.bin" BINARY', '  TRACK 01 MODE2/2352', '    INDEX 01 00:00:00']))
r, d = start(cue('ODMITNOUT_dobra2.cue', ['FILE "ODMITNOUT_dobra.bin" BINARY', '  TRACK 01 MODE2/2352', '    INDEX 01 00:00:00']))
over(r == 'PS1_HRA_FAIL jadro', 'O8) po vadnem obrazu dalsi start s dobrym -> "%s" (vada se neprenese)' % r)

r = LIB.h_start(PRAC.encode(), b'').decode()
LIB.h_stop()
over(r.startswith('PS1_BIOS_OK'), 'O9) start bez disku (BIOS): %s' % r)

vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb))
print(vystup[-1])
with open(os.path.join(ZDE, 'VYSTUP_test_b346_obraz.txt'), 'w', encoding='utf-8') as f:
    f.write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
