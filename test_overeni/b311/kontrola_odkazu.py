#!/usr/bin/env python3
# B311: ODKAZY V APPCE - nic nesmi vest na soubor, ktery v appce neni.
#  1) relativni odkazy v HTML appky (href, src, location.href / replace) i url(...) ve stylech
#     (pismo pisma/*.woff2, logo, obrazky karet) vedou na existujici soubor,
#  2) cesty k assetum v Jave a C++ (file:///android_asset/..., getAssets().open, AAssetManager_open,
#     nactiAsset / nacti v C++ menu) existuji (krome adres, ktere sklada Java sama),
#  3) menu/karty.txt: obrazek a cil kazde karty existuje (web a akce:... se nekontroluji),
#  4) obrazek stareho menu (nap_ui/nap_vision_intro_screen1.png) v appce uz neni a nic na nej nevede.
#   python3 kontrola_odkazu.py            -> VYSTUP_kontrola_odkazu.txt
import glob
import os
import re
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
M = os.path.normpath(os.path.join(ZDE, '../../app/src/main'))
A = M + '/assets'
vystup = []
chyb = 0


def tisk(x):
    print(x)
    vystup.append(x)


def ven(u):
    return re.match(r'^(https?:|data:|javascript:|mailto:|about:|ahgame:|file:|blob:|//|#)', u) or '+' in u or not u.strip()


n1 = n1c = 0
for f in sorted(glob.glob(A + '/**/*.html', recursive=True)):
    t = open(f, encoding='utf-8', errors='replace').read()
    for rx in (r'''(?:href|src)\s*=\s*["']([^"'#?]+)''', r'''location\.(?:replace\(|href\s*=\s*)["']([^"'#?]+)'''):
        for m in re.finditer(rx, t):
            u = m.group(1)
            if ven(u):
                continue
            n1 += 1
            if not os.path.exists(os.path.normpath(os.path.join(os.path.dirname(f), u))):
                chyb += 1
                tisk('CHYBA  odkaz %s -> %s' % (os.path.relpath(f, A), u))
    # url(...) ve stylech (ne v JavaScriptu generovane retezce s +)
    for m in re.finditer(r'''url\(\s*["']?([^"')]+?)["']?\s*\)''', t):
        u = m.group(1)
        if ven(u) or u.startswith('/'):
            continue
        n1c += 1
        if not os.path.exists(os.path.normpath(os.path.join(os.path.dirname(f), u))):
            chyb += 1
            tisk('CHYBA  url() %s -> %s' % (os.path.relpath(f, A), u))

virtualni = {'atari_xex_bridge.html', 'atarihelp_bridge.html', 'intro/etapa'}
n2 = 0
for f in glob.glob(M + '/java/**/*.java', recursive=True) + glob.glob(M + '/cpp/**/*.cpp', recursive=True) + glob.glob(M + '/cpp/menu/*.h'):
    if '/vendor/' in f:
        continue
    t = open(f, encoding='utf-8', errors='replace').read()
    cesty = set(re.findall(r'file:///android_asset/([A-Za-z0-9_./-]+)', t))
    cesty |= set(re.findall(r'(?:atariCppAsset|getAssets\(\)\.open|AAssetManager_open\([^,]*,|nactiAsset|nacti)\s*\(?\s*"([A-Za-z0-9_./-]+\.[A-Za-z0-9]+)"', t))
    for c in sorted(cesty):
        c = c.rstrip('/')
        if c in virtualni:
            continue
        n2 += 1
        if not os.path.exists(os.path.join(A, c)):
            chyb += 1
            tisk('CHYBA  asset %s -> %s' % (os.path.basename(f), c))

n3 = 0
for r in open(A + '/menu/karty.txt', encoding='utf-8'):
    r = r.strip()
    if not r or r.startswith('#'):
        continue
    p = [x.strip() for x in r.split('|')]
    for c in (p[4], p[5]):
        if not c or ven(c) or c.startswith('akce:'):
            continue
        n3 += 1
        if not os.path.exists(os.path.join(A, c)):
            chyb += 1
            tisk('CHYBA  karty.txt %s -> %s' % (p[0], c))

stary = 'nap_vision_intro_screen1'
odkazy = [f for f in glob.glob(M + '/**/*', recursive=True)
          if os.path.isfile(f) and f.endswith(('.html', '.js', '.java', '.h', '.cpp', '.txt', '.xml', '.css'))
          and stary in open(f, encoding='utf-8', errors='replace').read()]
if os.path.exists(A + '/nap_ui/' + stary + '.png') or odkazy:
    chyb += 1
    tisk('CHYBA  obrazek stareho menu: soubor %s, odkazy %s' % (os.path.exists(A + '/nap_ui/' + stary + '.png'), odkazy))

tisk('relativnich odkazu v HTML: %d, url() ve stylech: %d, cest k assetum v Jave a C++: %d, souboru z karty.txt: %d' % (n1, n1c, n2, n3))
tisk('VYSLEDEK: %d chyb' % chyb)
open(os.path.join(ZDE, 'VYSTUP_kontrola_odkazu.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
