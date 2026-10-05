#!/usr/bin/env python3
# B291: vygeneruje nap_atari_fonts2.h (Chakra Petch Medium + Space Mono Regular)
# a nap_atari_logo.h (N&P logo jako RGBA pole) pro C++ vykreslovani zarizeni
# Atari 130XE v HELP. Spusteni: python3 gen_b291_assets.py <adresar_s_ttf> <repo_root>
import sys, os
from PIL import Image
fontdir, root = sys.argv[1], sys.argv[2]
atari = os.path.join(root, 'app/src/main/cpp/atari')

def arr(name, data):
    out = ['static const unsigned char %s[%d] = {' % (name, len(data))]
    for i in range(0, len(data), 24):
        out.append(','.join(str(b) for b in data[i:i+24]) + ',')
    out.append('};')
    out.append('static const unsigned int %s_LEN = %d;' % (name, len(data)))
    return '\n'.join(out)

hdr = ['// nap_atari_fonts2.h - VYGENEROVANO (test_overeni/b291/gen_b291_assets.py), needitovat rucne.',
       '// B291: dalsi dva rezy pisma ze schvaleneho navrhu (Main.dc.html):',
       '//  - Chakra Petch Medium (500) - navrh nacita wght 500 a prohlizec ho pouzije pro',
       '//    texty bez font-weight (sipka pod okenkem kazety, symboly na tlacitkach kazety).',
       '//  - Space Mono Regular (400) - trida .mono v navrhu: pocitadlo "000.0" a napis',
       '//    "AUTOMATIC STOP" v okenku kazety.',
       '// Zdroj: github.com/google/fonts (ofl/chakrapetch, ofl/spacemono). Licence SIL OFL 1.1',
       '// (vendor/chakra_petch/OFL.txt, vendor/space_mono/OFL.txt).',
       '#pragma once', '']
for nm, fn in [('NAP_FONT_CHAKRA_MEDIUM_TTF', 'ChakraPetch-Medium.ttf'),
               ('NAP_FONT_SPACEMONO_REGULAR_TTF', 'SpaceMono-Regular.ttf')]:
    hdr.append(arr(nm, open(os.path.join(fontdir, fn), 'rb').read()))
    hdr.append('')
open(os.path.join(atari, 'nap_atari_fonts2.h'), 'w').write('\n'.join(hdr))

im = Image.open(os.path.join(root, 'app/src/main/assets/design/nap_logo_original.png')).convert('RGBA')
W = 192; H = round(im.size[1] * W / im.size[0])
small = im.convert('RGBa').resize((W, H), Image.LANCZOS).convert('RGBA')
data = small.tobytes()
lh = ['// nap_atari_logo.h - VYGENEROVANO (test_overeni/b291/gen_b291_assets.py), needitovat rucne.',
      '// B291: skutecne logo N&P studia (assets/design/nap_logo_original.png, 516x520 - stejne',
      '// logo jako v java/JS emu Atari) zmensene na %dx%d RGBA (Lanczos, premultiplikovane',
      '// alfa). C++ ho pri kresleni jeste jednou plynule zmensi na cilovou velikost.',
      '#pragma once', '',
      'static const int NAP_LOGO_W = %d, NAP_LOGO_H = %d;' % (W, H),
      arr('NAP_LOGO_RGBA', data), '']
lh[2] = lh[2] % (W, H)
open(os.path.join(atari, 'nap_atari_logo.h'), 'w').write('\n'.join(lh))
print('ok fonts2 + logo', W, H, 'alpha range', small.getextrema()[3])
