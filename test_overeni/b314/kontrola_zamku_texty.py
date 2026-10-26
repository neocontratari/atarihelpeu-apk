#!/usr/bin/env python3
# B314: DUVOD ZAMKU POD KARTOU - Java (NapPravidla.duvodKarty -> C++ menu) a zalozni menu v HTML
# (assets/index.html zamekKarty) musi rikat PRESNE totez, cesky i anglicky.
# Java: TestB314Java texty (prelozeno do JVM_ADRESAR), JavaScript: funkce radku + zamekKarty
# vytazene z index.html a spustene v Node (EN() / J() jako v strance).
#   python3 kontrola_zamku_texty.py JVM_ADRESAR      -> VYSTUP_kontrola_zamku_texty.txt
import json
import os
import re
import subprocess
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
INDEX = os.path.normpath(os.path.join(ZDE, '../../app/src/main/assets/index.html'))
jvm = sys.argv[1]
JAVA = os.environ.get('JAVA_HOME', '/usr/lib/jvm/java-21-openjdk-amd64') + '/bin/java'

radky = subprocess.run([JAVA, '-Dstdout.encoding=UTF-8', '-cp', jvm, 'eu.atarihelp.emu10.TestB314Java', 'texty'],
                       capture_output=True, text=True, encoding='utf-8', check=True).stdout.splitlines()
pripady = []
for r in radky:
    if r.count('|') < 5:
        continue
    jaz, kon, sega, rad, mi, text = r.split('|', 5)
    pripady.append({'jaz': jaz, 'zamek': kon.lower(), 'sega': sega == 'true', 'r': int(rad), 'm': int(mi), 'java': text})

h = open(INDEX, encoding='utf-8').read()
f_radku = re.search(r'function radku\(n\)\{.*?\n', h).group(0)
i = h.index('function zamekKarty(k,st){')
j = h.index('\n}\n', i) + 3
f_zamek = h[i:j]
js = '''
var JAZ = 'cs';
function EN() { return JAZ === 'en'; }
function J(cs, en) { return EN() ? en : cs; }
%s
%s
var P = %s, out = [];
P.forEach(function (p) {
  JAZ = p.jaz;
  var st = {sega: p.sega, ps1: false, zbyvaRadku: p.r, zbyvaMinut: p.m};
  if (p.zamek === 'sega') st.sega = false;
  out.push(zamekKarty({zamek: p.zamek}, st));
});
process.stdout.write(JSON.stringify(out));
''' % (f_radku, f_zamek, json.dumps([{k: v for k, v in p.items() if k != 'java'} for p in pripady]))
vys = json.loads(subprocess.run(['node', '-e', js], capture_output=True, text=True, encoding='utf-8', check=True).stdout)

vystup, chyb = [], 0
for p, t in zip(pripady, vys):
    ok = t == p['java']
    chyb += 0 if ok else 1
    vystup.append('%s %s %-4s r=%-2d m=%-2d %s' % ('OK    ' if ok else 'CHYBA ', p['jaz'], p['zamek'], p['r'], p['m'],
                                                 p['java'] if ok else 'Java "%s" / HTML "%s"' % (p['java'], t)))
vystup.append('VYSLEDEK: %d vet, %d nesouhlasi (Java = C++ menu, HTML = zalozni menu)' % (len(pripady), chyb))
print('\n'.join(vystup))
open(os.path.join(ZDE, 'VYSTUP_kontrola_zamku_texty.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb or not pripady else 0)
