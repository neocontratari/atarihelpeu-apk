#!/usr/bin/env python3
# ============================================================================
#  KONTROLA STRANKY PRO TV - podle predavaciho protokolu, bod 3d
#
#     "vytahnout retezce z Javy, SLOZIT A SPUSTIT v node
#      s napodobeninou prohlizece"
#     "POZOR: MUSI SE NEJDRIV ODSTRANIT JAVA KOMENTARE - v komentarich
#      jsou taky uvozovky a bez toho vznikne falesna chyba"
#
#  Delal jsem jen node --check. To najde chybejici zavorku, ale NE to,
#  ze se stranka rozbije az za behu. Tohle ji SPOUSTI.
# ============================================================================
import io, re, subprocess, sys

CESTA = sys.argv[1] if len(sys.argv) > 1 else \
        'b124/app/src/main/java/eu/atarihelp/emu10/MainActivity.java'
s = io.open(CESTA, encoding='utf-8').read()
i = s.index('private void napTvWebWriteHtml')
j = s.index('</body></html>";', i) + len('</body></html>";')

# 1) ODSTRANIT JAVA KOMENTARE
radky = [r for r in s[i:j].split('\n')
         if not r.lstrip().startswith(('//', '*', '/*'))]
cisty = '\n'.join(radky)

# 2) SLOZIT
kusy = re.findall(r'\+ "((?:[^"\\]|\\.)*)"', cisty)
prvni = re.search(r'String body = "((?:[^"\\]|\\.)*)"', cisty)
html = (prvni.group(1) if prvni else '') + ''.join(kusy)
html = (html.replace('\\"', '"').replace('\\\\', '\\')
            .replace('\\r', '\r').replace('\\n', '\n'))
io.open('/tmp/tv_stranka.html', 'w', encoding='utf-8').write(html)
skripty = re.findall(r'<script>([\s\S]*?)</script>', html)
print(f'  stranka {len(html)} znaku, {len(skripty)} skriptu')

NAPODOBA = io.open('/home/claude/work/prohlizec.js', encoding='utf-8').read()
chyb = 0
for n, sk in enumerate(skripty, 1):
    io.open(f'/tmp/tv_s{n}.js', 'w', encoding='utf-8').write(sk)
    r = subprocess.run(['node','--check',f'/tmp/tv_s{n}.js'], capture_output=True, text=True)
    if r.returncode != 0:
        chyb += 1; print(f'    skript {n}: SYNTAX CHYBA')
        for x in r.stderr.split('\n')[:3]:
            if x.strip(): print('      ', x.strip()[:92])
        continue
    beh = (NAPODOBA + '\ntry{\n' + sk + '\n}catch(e){ chyby.push("PRI SPUSTENI: "+(e&&e.message||e)); }\n'
           + 'for(var k=0;k<3;k++){ var d=casovace.slice(); casovace.length=0;\n'
           + '  for(var q=0;q<d.length;q++){ try{ d[q].f(); }catch(e){ chyby.push("V CASOVACI: "+(e&&e.message||e)); } } }\n'
           + 'if(chyby.length){ process.stderr.write(chyby.join(" | ")); process.exit(3); }\n')
    io.open(f'/tmp/tv_beh{n}.js','w',encoding='utf-8').write(beh)
    r2 = subprocess.run(['node', f'/tmp/tv_beh{n}.js'], capture_output=True, text=True)
    if r2.returncode == 0:
        print(f'    skript {n}: 0 chyb, SPUSTEN ({len(sk)} znaku)')
    else:
        chyb += 1; print(f'    skript {n}: SPADL PRI SPUSTENI')
        for x in (r2.stderr or r2.stdout).split('\n')[:4]:
            if x.strip(): print('      ', x.strip()[:92])
sys.exit(1 if chyb else 0)
