#!/usr/bin/env python3
"""B314: anglicky obrazek Segy (emu_sega/sega_megadrive_screen_en.png).

Na obrazku Segy jsou dole namalovane ceske popisky dlazdic SBIRKA / ULOZENE / HRY / NAVODY.
Tady se smazou (body textu + 2 body okolo, doplni je OpenCV inpaint z okoli - pozadi
i modra zare okraje dlazdice zustanou) a napisou se
anglicke CATALOG / SAVES / GAMES / MANUALS stejnym pismem (velikost jako WEB a MENU na
stejnem obrazku: Liberation Sans Bold = metriky Arial Bold, 21 bodu na sirce 941),
stejnou barvou (prechod shora 226 -> dole 175 sedi) se stejnym vyhlazenim.
WEB a MENU zustanou. Pruhlednost (alfa) je presne z puvodniho obrazku - pod polopruhlednymi
body skla obrazovky prosvita hra jako v cestine.
Pouziti: python3 sega_skin_en.py  (vstup i vystup v assets/emu_sega)
"""
import os
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ZDE = os.path.dirname(os.path.abspath(__file__))
A = os.path.normpath(os.path.join(ZDE, '../../app/src/main/assets/emu_sega'))
VSTUP = os.path.join(A, 'sega_megadrive_screen.png')
VYSTUP = os.path.join(A, 'sega_megadrive_screen_en.png')
PISMO = '/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf'

# (stred x, puvodni okraj textu x0..x1, novy text) - namereno na obrazku 941 x 1672
POPISKY = [
    (87.0, 49, 125, 'CATALOG'),
    (242.0, 194, 290, 'SAVES'),
    (389.5, 366, 413, 'GAMES'),
    (544.0, 502, 586, 'MANUALS'),
]
Y_HORNI, Y_SPODNI = 1594, 1618          # pas, kde je text (vcetne hacku a carek)
CAP_Y0, CAP_Y1 = 1601, 1615             # horni a dolni hrana velkych pismen (jako WEB / MENU)


def main():
    puv = Image.open(VSTUP)
    alfa = np.asarray(puv.convert('RGBA'))[:, :, 3].copy()   # pruhlednost (sklo obrazovky) zustane
    im = puv.convert('RGB')
    a = np.asarray(im).astype(np.uint8)
    # maska puvodniho textu: svetle sede body (malo barvy) v pasu popisku, rozsirena o 2 body
    # (vyhlazeni okraju), pak inpainting (OpenCV Telea) - pozadi i modra zare okraje dlazdice zustanou
    import cv2
    maska = np.zeros(a.shape[:2], np.uint8)
    for cx, x0, x1, _ in POPISKY:
        pas = a[Y_HORNI:Y_SPODNI + 1, x0 - 2:x1 + 3].astype(np.int32)
        lum = pas.mean(axis=2)
        sat = pas.max(axis=2) - pas.min(axis=2)
        m = ((lum > 40) & (sat < 70)).astype(np.uint8) * 255
        maska[Y_HORNI:Y_SPODNI + 1, x0 - 2:x1 + 3] = m
    maska = cv2.dilate(maska, np.ones((5, 5), np.uint8))
    bgr = cv2.cvtColor(a, cv2.COLOR_RGB2BGR)
    bgr = cv2.inpaint(bgr, maska, 6, cv2.INPAINT_TELEA)
    out = cv2.cvtColor(bgr, cv2.COLOR_BGR2RGB).astype(np.float32)
    # novy text: vykreslit 4x vetsi a zmensit (vyhlazeni jako na obrazku)
    S = 4
    pismo = ImageFont.truetype(PISMO, 21 * S)
    for cx, x0, x1, text in POPISKY:
        maska = Image.new('L', (220 * S, 40 * S), 0)
        d = ImageDraw.Draw(maska)
        bb = d.textbbox((0, 0), text, font=pismo)
        w = bb[2] - bb[0]
        # velka pismena: horni hrana bb[1] -> CAP_Y0
        d.text((-bb[0] + (220 * S - w) / 2.0, -bb[1] + 6 * S), text, font=pismo, fill=255)
        m = np.asarray(maska.resize((220, 40), Image.LANCZOS)).astype(np.float32) / 255.0
        ox = int(round(cx - 110))
        oy = CAP_Y0 - 6
        h, wm = m.shape
        for yy in range(h):
            y = oy + yy
            # barva podle radku: 226 nahore -> 175 dole
            k = min(1.0, max(0.0, (y - CAP_Y0) / float(CAP_Y1 - CAP_Y0)))
            col = np.array([226, 226, 227]) * (1 - k) + np.array([175, 176, 178]) * k
            for xx in range(wm):
                al = m[yy, xx]
                if al <= 0.003:
                    continue
                x = ox + xx
                out[y, x, :3] = out[y, x, :3] * (1 - al) + col * al
    rgba = np.dstack([np.clip(out + 0.5, 0, 255).astype(np.uint8), alfa])
    Image.fromarray(rgba, 'RGBA').save(VYSTUP, optimize=True)
    print('ulozeno', VYSTUP, os.path.getsize(VYSTUP), 'B')


if __name__ == '__main__':
    main()
