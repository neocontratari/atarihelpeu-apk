#!/usr/bin/env python3
# B310: po smazani stareho Atari nesmi nic odkazovat na smazane soubory.
#  1) relativni odkazy v HTML appky (href, src, location.replace / href) vedou na existujici soubor,
#  2) cesty k assetum v Jave a C++ (file:///android_asset/..., atariCppAsset, AAssetManager_open)
#     existuji (krome adres stranek, ktere sklada Java sama: atari_xex_bridge.html,
#     atarihelp_bridge.html, a predpony intro/etapa + cislo),
#  3) v kodu appky (Java, C++, HTML) uz neni odkaz na emu_vbxe, emu/, emulator/, emu1/, emu_1/,
#     emu10_130xe.html ani obrazky vbxe_130xe_*_exact.png (komentare s B310 / historie se nepocitaji).
import glob, os, re, sys
M = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../app/src/main'))
A = M + '/assets'
chyb = 0; n1 = 0; n2 = 0
for f in glob.glob(A + '/**/*.html', recursive=True):
    t = open(f, encoding='utf-8', errors='replace').read()
    for rx in (r'''(?:href|src)\s*=\s*["']([^"'#?]+)''', r'''location\.(?:replace\(|href\s*=\s*)["']([^"'#?]+)'''):
        for m in re.finditer(rx, t):
            u = m.group(1)
            if re.match(r'^(https?:|data:|javascript:|mailto:|about:|ahgame:|file:|//)', u) or '+' in u or not u.strip():
                continue
            n1 += 1
            if not os.path.exists(os.path.normpath(os.path.join(os.path.dirname(f), u))):
                chyb += 1; print('CHYBA  odkaz', os.path.relpath(f, A), '->', u)
virtualni = {'atari_xex_bridge.html', 'atarihelp_bridge.html', 'intro/etapa'}
for f in glob.glob(M + '/java/**/*.java', recursive=True) + glob.glob(M + '/cpp/**/*.cpp', recursive=True):
    t = open(f, encoding='utf-8', errors='replace').read()
    cesty = set(re.findall(r'file:///android_asset/([A-Za-z0-9_./-]+)', t))
    cesty |= set(re.findall(r'(?:atariCppAsset|getAssets\(\)\.open|AAssetManager_open\([^,]*,)\s*\(?\s*"([A-Za-z0-9_./-]+)"', t))
    for c in sorted(cesty):
        c = c.rstrip('/')
        if c in virtualni:
            continue
        n2 += 1
        if not os.path.exists(os.path.join(A, c)):
            chyb += 1; print('CHYBA  asset', os.path.basename(f), '->', c)
stare = re.compile(r'emu_vbxe/|emu/index|emulator/index|emu1/|emu_1/|emu10_130xe|vbxe_130xe_joystick_exact|vbxe_130xe_keyboard_exact')
n3 = 0
for f in glob.glob(M + '/java/**/*.java', recursive=True) + glob.glob(M + '/cpp/atari/*', recursive=True) + glob.glob(M + '/cpp/*.cpp') + glob.glob(A + '/**/*.html', recursive=True) + glob.glob(A + '/**/*.js', recursive=True):
    for i, r in enumerate(open(f, encoding='utf-8', errors='replace'), 1):
        if stare.search(r):
            s = r.strip()
            if 'B310' in s or s.startswith('//') or s.startswith('*') or 'git show' in s:
                continue
            n3 += 1; chyb += 1
            print('CHYBA  stary odkaz', os.path.relpath(f, M) + ':' + str(i), s[:120])
print('relativnich odkazu v HTML: %d, cest k assetum v kodu: %d, odkazu na smazane soubory: %d' % (n1, n2, n3))
print('VYSLEDEK: %d chyb' % chyb)
sys.exit(1 if chyb else 0)
