#!/usr/bin/env python3
"""B341: kontrola zapojeni (JEDNA APPKA po aktualizaci, ovladac a dalkovy ve hre Segy podle stranky, seznam BT
ovladacu v OPTIONS se neprekresluje porad dokola).
Pouziti: python3 test_overeni/b341/kontrola_b341.py   -> VYSTUP_kontrola_b341.txt
"""
import os
import re
import subprocess
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
R = os.path.normpath(os.path.join(ZDE, '../..'))


def cti(p):
    return open(os.path.join(R, p), encoding='utf-8').read()


MA = cti('app/src/main/java/eu/atarihelp/emu10/MainActivity.java')
PA = cti('app/src/main/java/eu/atarihelp/emu10/NapPoAktualizaci.java')
IX = cti('app/src/main/assets/index.html')
GR = cti('app/build.gradle')
vystup, chyb = [], 0


def over(ok, co):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)


def blok(text, zac, kon):
    i = text.index(zac)
    return text[i:text.index(kon, i + len(zac))]


# ---- A) jedna appka ----
oc = blok(MA, '    protected void onCreate(Bundle savedInstanceState) {', 'napTvWebLogFileInit();')
over(oc.index('super.onCreate(savedInstanceState);') < oc.index('!isTaskRoot()')
     and 'Intent.ACTION_MAIN.equals(ii.getAction())' in oc
     and '(ii.hasCategory(Intent.CATEGORY_LAUNCHER) || ii.hasCategory(Intent.CATEGORY_LEANBACK_LAUNCHER))' in oc
     and oc.index('napJenDuplikat = true;') < oc.index('finish();') < oc.index('return;'),
     'A1 onCreate: druha appka z ikony (ne koren ulohy, MAIN + LAUNCHER / LEANBACK_LAUNCHER) se hned zavre - PRED logem a vsim ostatnim')
od = blok(MA, '    protected void onDestroy() {', '\n    }\n')
over(od.strip().split('\n')[1].strip() == 'if (napJenDuplikat) { super.onDestroy(); return; }   // B341: druha appka z ikony - nic nezastavovat'
     and 'stopNativeInPlaceHard("activityDestroy");' in od and 'NativeAtariCoreBridge.devStopSafe();' in od,
     'A2 onDestroy: zavrena druha appka NEZASTAVI jadra / server / ovladac bezici appky (staticke mosty) - kontrola hned na zacatku')
over('private static volatile int napDruhaAppkaZavrena = 0;' in MA and 'private boolean napJenDuplikat = false;' in MA
     and '"B341 JEDNA APPKA: ikona chtela spustit druhou appku nad bezici ("' in MA,
     'A3 bezici appka v onResume zapise do logu, ze se druha z ikony zavrela (pocitadlo je staticke - spolecne obema)')
over('Intent o = new Intent(c, MainActivity.class);' in PA and 'o.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);' in PA,
     'A4 NapPoAktualizaci (B327) beze zmeny: po aktualizaci appku otevre jinym intentem nez ikona (proto A1)')

# ---- O) ovladac ve hre Segy ----
ok = blok(MA, '    private int ovladacKam() {', '\n    }\n')
over('isSegaNativeOwnerUrl(napAktualniUrl)' in ok and 'isSegaNativeOwnerUrl(napTvWebCurrentUrl)' not in ok
     and 'napAktualniUrl = url == null ? "" : url;' in MA,
     'O1 ovladacKam: Sega podle adresy stranky z onPageStarted (napAktualniUrl), ne podle cache vysilani WEB TV')
over(MA.count('napTvWebCurrentUrl = curUrl;') == 1, 'O2 cache WEB TV (napTvWebCurrentUrl) dal jen pro vysilani na PC / TV - beze zmeny')

# ---- S) seznam BT ovladacu ----
pk = blok(IX, 'function btPrekresli(el,podpis,kresli){', '\n}\n')
over("if(el.getAttribute('data-podpis')===podpis) return false;" in pk and "NapTvOvladani.fokus()" in pk
     and "getAttribute('data-a')===fa" in pk and 'NapTvOvladani.vyber(el.children[i])' in pk,
     'S1 btPrekresli: prekreslit jen pri zmene seznamu; pri zmene ramecek dalkoveho zustane na tomtez zarizeni (data-a)')
bo = blok(IX, 'function btObnov(){', '\n}\n')
over(bo.count('btPrekresli(') == 2 and 'while(sez.firstChild)' not in bo and 'while(zs.firstChild)' not in bo
     and "b.setAttribute('data-a',d.a||'');" in IX and "b.setAttribute('data-a',x.a);" in bo,
     'S2 btObnov: nalezene i zapamatovane ovladace pres btPrekresli (drive smazat a nakreslit kazdych 0,7 s)')
js = '\n'.join(re.findall(r'<script>([\s\S]*?)</script>', IX))
r = subprocess.run(['node', '-e', "const a=require('/opt/npm-tools/node_modules/acorn');a.parse(process.argv[1],{ecmaVersion:5});console.log('ok')", js],
                   capture_output=True, text=True, timeout=60)
over(r.stdout.strip() == 'ok', 'S3 skripty index.html jsou ES5%s' % ('' if r.stdout.strip() == 'ok' else ': ' + r.stderr[-200:]))

over('versionCode 384' in GR and 'versionName "EMU10-B341-JEDNA-APPKA-OVLADAC"' in GR, 'N1 build.gradle: versionCode 384 "EMU10-B341-JEDNA-APPKA-OVLADAC"')

vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (sum(1 for v in vystup if v.startswith(('OK', 'CHYBA'))), chyb))
print('\n'.join(vystup))
open(os.path.join(ZDE, 'VYSTUP_kontrola_b341.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
