#!/usr/bin/env python3
"""B334: banner appky pro nabidku Android TV (android:banner, 320 x 180 bodu, drawable-xhdpi).
Ikona appky (zlata mince N&P z mipmap-xxxhdpi) vlevo, vpravo "N&P VISION" pismem appky (Chakra Petch,
assets/pisma, licence OFL - prevedeno z woff2 pres fontTools).
Pouziti: python3 udelej_banner.py PRAC   -> app/src/main/res/drawable-xhdpi/nap_tv_banner.png
"""
import os
import sys

from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ZDE = os.path.dirname(os.path.abspath(__file__))
R = os.path.normpath(os.path.join(ZDE, '../..'))
PRAC = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else '.')
os.makedirs(PRAC, exist_ok=True)


def ttf(jmeno):
    cil = os.path.join(PRAC, jmeno + '.ttf')
    f = TTFont(os.path.join(R, 'app/src/main/assets/pisma', jmeno + '.woff2'))
    f.flavor = None
    f.save(cil)
    return cil


W, H, K = 320, 180, 4            # kresli se 4x vetsi a zmensi (hladke hrany)
img = Image.new('RGB', (W * K, H * K), (18, 19, 24))
d = ImageDraw.Draw(img)
# pozadi: tmavy prechod jako ikona (#1D1E23) + jemna mrizka
for y in range(H * K):
    t = y / (H * K)
    c = (int(16 + 14 * (1 - abs(t - 0.45) * 2)), int(17 + 14 * (1 - abs(t - 0.45) * 2)), int(22 + 16 * (1 - abs(t - 0.45) * 2)))
    d.line([(0, y), (W * K, y)], fill=c)
for x in range(0, W * K, 9 * K):
    d.line([(x, 0), (x, H * K)], fill=(10, 10, 13), width=K)
for y in range(0, H * K, 9 * K):
    d.line([(0, y), (W * K, y)], fill=(10, 10, 13), width=K)
# zlata linka dole
for i in range(3 * K):
    a = 1 - i / (3 * K)
    d.line([(0, H * K - 1 - i), (W * K, H * K - 1 - i)], fill=(int(120 * a + 18), int(96 * a + 19), int(40 * a + 24)))

ikona = Image.open(os.path.join(R, 'app/src/main/res/mipmap-xxxhdpi/ic_launcher.png')).convert('RGBA')
vel = 140 * K
ik = ikona.resize((vel, vel), Image.LANCZOS)
stin = Image.new('RGBA', (vel + 40 * K, vel + 40 * K), (0, 0, 0, 0))
ImageDraw.Draw(stin).rounded_rectangle([20 * K, 24 * K, vel + 20 * K, vel + 24 * K], radius=22 * K, fill=(0, 0, 0, 150))
stin = stin.filter(ImageFilter.GaussianBlur(8 * K))
x0, y0 = 14 * K, (H * K - vel) // 2 - 4 * K
img.paste(stin, (x0 - 20 * K, y0 - 20 * K), stin)
img.paste(ik, (x0, y0), ik)

f1 = ImageFont.truetype(ttf('chakra_bold'), 58 * K)
f2 = ImageFont.truetype(ttf('chakra_semibold'), 27 * K)
f3 = ImageFont.truetype(ttf('chakra_semibold'), 12 * K)


def zlaty_text(xy, text, font, roztec=0):
    """Text se zlatym prechodem shora dolu a tmavym stinem."""
    x, y = xy
    maska = Image.new('L', img.size, 0)
    dm = ImageDraw.Draw(maska)
    stinm = Image.new('L', img.size, 0)
    ds = ImageDraw.Draw(stinm)
    cx = x
    for ch in text:
        dm.text((cx, y), ch, font=font, fill=255)
        ds.text((cx + 2 * K, y + 3 * K), ch, font=font, fill=200)
        cx += font.getlength(ch) + roztec
    stinm = stinm.filter(ImageFilter.GaussianBlur(2 * K))
    img.paste((0, 0, 0), (0, 0), stinm)
    box = maska.getbbox() or (0, 0, 1, 1)
    grad = Image.new('RGB', img.size)
    dg = ImageDraw.Draw(grad)
    for yy in range(box[1], box[3] + 1):
        t = (yy - box[1]) / max(1, box[3] - box[1])
        # svetle zlato nahore, sytejsi dole (#FFE3A6 -> #D9A441)
        dg.line([(box[0], yy), (box[2], yy)], fill=(int(255 - 38 * t), int(227 - 63 * t), int(166 - 101 * t)))
    img.paste(grad, (0, 0), maska)
    return cx


xt = 168 * K
zlaty_text((xt, 30 * K), 'N&P', f1)
zlaty_text((xt + 2 * K, 98 * K), 'VISION', f2, roztec=3 * K)
d = ImageDraw.Draw(img)
d.text((xt + 3 * K, 138 * K), '© 2026', font=f3, fill=(170, 150, 110))

ven = img.resize((W, H), Image.LANCZOS)
cil = os.path.join(R, 'app/src/main/res/drawable-xhdpi/nap_tv_banner.png')
os.makedirs(os.path.dirname(cil), exist_ok=True)
ven.save(cil, optimize=True)
print('banner %dx%d -> %s (%d B)' % (W, H, cil, os.path.getsize(cil)))
