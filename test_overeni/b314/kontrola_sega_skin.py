#!/usr/bin/env python3
# B314: anglicky obrazek konzole Sega (assets/emu_sega/sega_megadrive_screen_en.png, z sega_skin_en.py)
# proti ceskemu: stejny rozmer, RGBA a PRESNE stejna pruhlednost (polopruhledne sklo obrazovky -
# pod nim prosvita hra), body jine jen v pasu popisku dlazdic dole (CATALOG / SAVES / GAMES / MANUALS),
# stranka Segy ma u obrazku data-en-src (prekladac stranek ho anglicky vymeni).
#   python3 kontrola_sega_skin.py      -> VYSTUP_kontrola_sega_skin.txt
import os
import sys

import numpy as np
from PIL import Image

ZDE = os.path.dirname(os.path.abspath(__file__))
A = os.path.normpath(os.path.join(ZDE, '../../app/src/main/assets/emu_sega'))
cz = Image.open(os.path.join(A, 'sega_megadrive_screen.png'))
en = Image.open(os.path.join(A, 'sega_megadrive_screen_en.png'))
vystup, chyb = [], 0


def over(ok, co):
    global chyb
    chyb += 0 if ok else 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)


over(cz.size == en.size and en.mode == 'RGBA', 'rozmer %s / %s, anglicky %s' % (cz.size, en.size, en.mode))
a = np.asarray(cz.convert('RGBA')).astype(int)
b = np.asarray(en.convert('RGBA')).astype(int)
over((a[:, :, 3] == b[:, :, 3]).all(), 'pruhlednost stejna jako cesky (polopruhlednych bodu %d)' % int((a[:, :, 3] < 255).sum()))
jine = np.abs(a[:, :, :3] - b[:, :, :3]).sum(axis=2) > 0
ys, xs = np.where(jine)
mimo = int(((ys < 1588) | (ys > 1624)).sum()) if len(ys) else 0
over(len(ys) > 500 and mimo == 0, 'jine body jen v pasu popisku: %d bodu, y %d-%d, mimo pas %d' % (
    len(ys), ys.min() if len(ys) else -1, ys.max() if len(ys) else -1, mimo))
h = open(os.path.join(A, 'index.html'), encoding='utf-8').read()
over('src="sega_megadrive_screen.png" data-en-src="sega_megadrive_screen_en.png"' in h, 'stranka Segy: <img ... data-en-src="sega_megadrive_screen_en.png">')
vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb))
print('\n'.join(vystup))
open(os.path.join(ZDE, 'VYSTUP_kontrola_sega_skin.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
