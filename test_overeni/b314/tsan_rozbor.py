#!/usr/bin/env python3
# B314 (kopie ../b313): rozbor vystupu ThreadSanitizeru (test_menu_host_b314 prelozeny s -fsanitize=thread).
# Kazde hlaseni ma dva pristupy; rozhoduje PRVNI ramec mimo samotny TSan (interceptory
# pthread_*, malloc ...): kdyz je u obou pristupu v knihovne Mesa (libgallium, libEGL,
# libGLESv2, LLVM), je to zavod uvnitr Mesy (jeji vlakna llvmpipe - na telefonu Mesa neni).
# Kdyz je aspon jeden v nasem kodu (menu, proud na TV, test), je to NAS zavod = chyba.
#   python3 tsan_rozbor.py VYSTUP_TSAN.txt         -> VYSTUP_tsan_menu_host_b314.txt
import collections
import os
import re
import sys

t = open(sys.argv[1], encoding='utf-8', errors='replace').read()
bloky = [b for b in t.split('==================') if 'WARNING: ThreadSanitizer' in b]
MESA = re.compile(r'libgallium|libEGL|libGLESv2|libLLVM|libdrm|mesa|libglapi')
TSAN = re.compile(r'libsanitizer|libtsan')
nase, mesa = [], collections.Counter()
for b in bloky:
    druh = re.search(r'WARNING: ThreadSanitizer: ([^\n(]+)', b).group(1).strip()
    pristupy = re.split(r'\n  (?=(?:Previous )?(?:atomic )?(?:[Ww]rite|[Rr]ead)|Previous|Mutex|Location|Thread)', b)
    prvni = []
    for p in pristupy:
        if not re.match(r'(Previous )?(atomic )?([Ww]rite|[Rr]ead)', p.strip()):
            continue
        ramce = re.findall(r'\n\s+#\d+ (.*)', p)
        mimo = [r for r in ramce if not TSAN.search(r)]
        prvni.append(mimo[0] if mimo else (ramce[0] if ramce else '?'))
    if prvni and all(MESA.search(r) for r in prvni):
        mesa[druh] += 1
    else:
        nase.append((druh, prvni))

vystup = ['B314 ThreadSanitizer - test_menu_host_b314 (skutecny nap_menu_gl.cpp: GL vlakno menu, dotyk a klavesy',
          'z "Javy", proud na TV, snimek pro stary prohlizec, plocha pryc a zpet)',
          'test sam: ' + (re.search(r'VYSLEDEK: [^\n]*', t).group(0) if re.search(r'VYSLEDEK: [^\n]*', t) else '?'),
          'hlaseni celkem: %d' % len(bloky)]
for k, v in mesa.items():
    vystup.append('  uvnitr Mesy (llvmpipe, obe strany v libgallium - na telefonu neni): %d x %s' % (v, k))
vystup.append('  v NASEM kodu: %d' % len(nase))
for druh, prvni in nase:
    vystup.append('    %s: %s' % (druh, ' | '.join(x[:120] for x in prvni)))
vystup.append('VYSLEDEK: %d zavodu v nasem kodu' % len(nase))
print('\n'.join(vystup))
open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'VYSTUP_tsan_menu_host_b314.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if nase else 0)
