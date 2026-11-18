#!/usr/bin/env python3
"""B347: staticka kontrola zdroju - stranka ODEMKNOUT pozna "banka_auto" (server v12.3 cte e-maily od banky),
anglicke texty, log v Jave, verze, napoveda CZ / EN stejne stavby. Pouziti: python3 kontrola_b347.py
-> VYSTUP_kontrola_b347.txt"""
import os
import re

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '..', '..'))
A = os.path.join(KOREN, 'app', 'src', 'main')
cti = lambda *c: open(os.path.join(*c), encoding='utf-8').read()
vys, chyb = [], 0


def over(ok, co, detail=''):
    global chyb
    chyb += 0 if ok else 1
    vys.append(('OK     ' if ok else 'CHYBA  ') + co + ('' if ok or not detail else '   -> ' + str(detail)[:300]))


st = cti(A, 'assets', 'hry', 'odemknout.html')
over('id="sama"' in st and 'id="samaText"' in st and '🏦 ODEMKNE SE SAMO' in st and 'id="radaZaplatil"' in st,
     'S1 odemknout.html: okenko #sama (🏦 ODEMKNE SE SAMO) a rada #radaZaplatil')
over("var hlidat = !!d.zadost || !!d.banka_auto;" in st and 'if (hlidat !== !!hlidac) hlidej(hlidat);' in st,
     'S2 hlidani kazdych 20 s i bez ZAPLATIL JSEM, kdyz banka_auto')
over("(p.zadost || p.banka_auto)" in st and 'd.banka_auto, d.vs' in st, 'S3 navrat do appky se pta hned; zmena banka_auto prekresli stranku')
over("ukaz('sama', sama && !p.zadost);" in st, 'S4 po ZAPLATIL JSEM okenko SAMO zmizi (ČEKÁ uz to rika)')
en = cti(A, 'assets', 'jazyk', 'en_hry.js')
klice = re.findall(r"var (?:CEKA|RADA)_(?:RUCNE|SAMA) = '([^']+)';", st)
m = re.search(r'<div id="samaText">([^<]+)</div>', st)
klice += ['ODEMKNE SE SAMO'] + ([m.group(1).replace('&amp;', '&')] if m else [])
chybi = [k for k in klice if ("'" + k + "'") not in en]
over(len(klice) == 6 and not chybi, 'E1 en_hry.js: vsech 6 textu (2 puvodni + 4 nove B347)', chybi)
over("'ODEMKNE SE SAMO': 'UNLOCKS BY ITSELF'" in en, 'E2 ODEMKNE SE SAMO = UNLOCKS BY ITSELF')
ma = cti(A, 'java', 'eu', 'atarihelp', 'emu10', 'MainActivity.java')
over('private volatile int napBankaAutoLog = -1;' in ma and 'appendNativeLog("B347 PLATBA " + c + ": "' in ma
     and 'server pozna platbu z banky sam - stranka hlida kazdych 20 s' in ma, 'J1 MainActivity: log B347 PLATBA (jen zmena banka_auto)')
bg = cti(KOREN, 'app', 'build.gradle')
over('versionCode 390' in bg and 'versionName "EMU10-B347-PLATBA-Z-BANKY"' in bg, 'V build.gradle 390 EMU10-B347-PLATBA-Z-BANKY')
cz, enh = cti(A, 'assets', 'napoveda', 'index.html'), cti(A, 'assets', 'napoveda', 'en.html')


def blok(t, nadpis):
    i = t.find(nadpis)
    return t[i:t.index('</ul>', i)] if i >= 0 else ''


bc, be = blok(cz, 'ODEMKNOUT HNED – platba'), blok(enh, 'UNLOCK NOW – payment')
over(bc.count('<li>') == be.count('<li>') == 6, 'N1 napoveda: blok platby CZ i EN 6 polozek (nova o ODEMKNE SE SAMO)', (bc.count('<li>'), be.count('<li>')))
over('<span class="t">ODEMKNE SE SAMO</span>' in bc and '<span class="t">UNLOCKS BY ITSELF</span>' in be and 'třeba rodič' in bc and 'e.g. a parent' in be,
     'N2 napoveda CZ / EN: ODEMKNE SE SAMO / UNLOCKS BY ITSELF, platit muze i rodic')
over('S <span class="nw"><span class="t">ODEMKNE SE SAMO</span></span> server pozná platbu z banky sám' in cz
     and 'With <span class="nw"><span class="t">UNLOCKS BY ITSELF</span></span> the server sees the payment' in enh,
     'N3 otazka „Zaplatil jsem, ale konzole je pořád zamčená“ CZ / EN doplnena')
over(not re.search(r'csas|imap|dkim|seznam\.cz', st + en, re.I), 'P1 v appce zadne podrobnosti o bance ani schrance serveru')

vys.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vys), chyb))
open(os.path.join(ZDE, 'VYSTUP_kontrola_b347.txt'), 'w', encoding='utf-8').write('\n'.join(vys) + '\n')
print('\n'.join(vys))
