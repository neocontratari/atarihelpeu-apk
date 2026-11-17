#!/usr/bin/env python3
"""B337: kontrola zapojeni (TV: SEGA / PS1 zpet do menu, Sonic + BIOS na zacatku, Atari -> hlavni menu, log z TV).
Pouziti: python3 test_overeni/b337/kontrola_b337.py   -> VYSTUP_kontrola_b337.txt
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
ST = cti('app/src/main/java/eu/atarihelp/emu10/NapStahovaniSeSouhlasem.java')
GR = cti('app/build.gradle')
OD = cti('app/src/main/assets/hry/odemknout.html')
HR = cti('app/src/main/assets/hry/index.html')
TV = cti('app/src/main/assets/spolecne/nap_tv_ovladani.js')
IX = cti('app/src/main/assets/index.html')
EN = cti('app/src/main/assets/jazyk/en_menu.js')
NCZ = cti('app/src/main/assets/napoveda/index.html')
NEN = cti('app/src/main/assets/napoveda/en.html')
vystup, chyb = [], 0


def over(ok, co):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)


def blok(text, zac, kon):
    i = text.index(zac)
    return text[i:text.index(kon, i + len(zac))]


# ---- Z) SEGA / PS1: zamcena karta, stranka ODEMKNOUT ----
ma = blok(MA, '    private void menuAkce(int kod, String odkud) {', '\n    }\n')
over('if ("sega".equals(zamek) || "ps1".equals(zamek)) napZamcenaKarta(i, karta, zamek);' in ma
     and 'napOtevriOdemknuti(zamek, "karta menu"), 550)' not in ma,
     'Z1 zamcena karta v menu -> napZamcenaKarta (nejdriv server), ne rovnou ODEMKNOUT')
zk = blok(MA, '    private void napZamcenaKarta(final int i, final String karta, final String zamek) {', '\n    }\n')
over('napServer.get("/api/v1/stav")' in zk and 'napSlucServer(NapJson.mapa(o.json, "odemceno")' in zk
     and 'ui.postDelayed(odemknout, 2500);' in zk and 'menuSpust(i, karta);' in zk and 'rozhodnuto.compareAndSet(false, true)' in zk
     and 'Math.max(0L, 550L - ' in zk and 'B337 ZAMCENA KARTA' in zk,
     'Z2 napZamcenaKarta: /api/v1/stav + slouceni, zaplaceno -> menuSpust (konzole), jinak / do 2,5 s ODEMKNOUT, zatreseni 0,5 s zustava, log')
mu = blok(MA, '    private boolean menuUkaz(String proc) {', '\n    }\n')
zs = blok(MA, '    private void napZamkyZeServeru(String proc) {', '\n    }\n')
over('napZamkyZeServeru("menu " + proc);' in mu and 'NapPostup.segaOdemcena(this) && NapPostup.ps1Odemcena(this)) return;' in zs
     and '60_000L' in zs and 'napOverStav(proc, true);' in zs,
     'Z3 menu ukazane a neco zamcene -> stav ze serveru (nejvys 1x za minutu, tise)')
pl = blok(MA, '        @JavascriptInterface public void platba(final String co) {', '\n        }\n')
over('NapServerKlient.Odpoved s = napServer.get("/api/v1/stav");' in pl and 'NapJson.mapa(s.json, "odemceno")' in pl,
     'Z4 stranka ODEMKNOUT, server "zaplaceno" -> cely stav (Sega i PS1), ne jen tahle konzole')
over('private volatile long napStavCas = 0;' in MA, 'Z5 napStavCas volatile (pisou ho i vlakna)')
over("var TV_AKCE = ['spustit', 'naSegu', 'zkontroluj', 'zaplatil', 'znovu'];" in OD and 'if (tvCil === TV_AKCE[i]) return;' in OD
     and 'poVykresleni(); }' in OD and 'T.vyber(el)' in OD and '!window.NAP_TV' in OD,
     'Z6 ODEMKNOUT na TV: ramecek na hlavni akci (SPUSTIT ...), ne DO MENU; hlidani platby ho nebere; telefon beze zmeny')
over('vyber: function (el) {' in TV and "if (!W.NAP_TV && !aktivni) return '';" in TV,
     'Z7 NapTvOvladani.vyber(el) - stranka urci ramecek (jen TV / sipky)')

# ---- S) Sonic + BIOS ----
pd = blok(MA, '    private File getPublicAtariHelpDownloadsDir() {', '\n    }\n')
over('Build.VERSION.SDK_INT <= 28 && !napUlozistePovoleno()' in pd and 'napSlozkaAppky()' in pd,
     'S1 Android 9 a starsi bez povoleni uloziste -> soubory do slozky appky (Android 10+ a s povolenim beze zmeny)')
pv = blok(MA, '    private boolean napUlozistePovoleno() {', '\n    }\n')
over('SDK_INT > 28) return true;' in pv and 'Manifest.permission.WRITE_EXTERNAL_STORAGE' in pv, 'S2 povoleni zapisu se pta jen Android 6-9')
zu = blok(MA, '    private void napZajistiUloziste(Runnable pak) {', '\n    }\n')
rp = blok(MA, '    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {', '\n    }\n')
over('requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE' in zu and 'REQ_ULOZISTE_STAHOVANI' in zu
     and 'if (requestCode == REQ_ULOZISTE_STAHOVANI) {' in rp and 'if (r != null) ui.post(r);' in rp,
     'S3 STAHNOUT -> okno povoleni uloziste, po odpovedi (ano i ne) stazeni')
ss = blok(MA, '    private void napStahniSoubory(NapStahovaniSeSouhlasem.Hotovo pak) {', '\n    }\n')
over('this::getPublicAtariHelpDownloadsDir' in ss and 'this::napZajistiUloziste' in ss and 'this::napZalohaSouboru' in ss
     and MA.count('napStahniSoubory(zprava ->') == 2 and 'NapStahovaniSeSouhlasem.zeptejSeAStahni(MainActivity.this,' not in MA,
     'S4 start appky i OPTIONS -> STAHNOUT HRY A BIOS jdou pres napStahniSoubory (povoleni, slozka az po nem, zaloha N&P)')
zz = blok(MA, '    private NapStahovaniSeSouhlasem.Zdroj napZalohaSouboru(String co) throws Exception {', '\n    }\n')
over('napServer.get("/api/v1/hry")' in zz and 'NapStahovaniSeSouhlasem.vyberZeSeznamu(seznam, co)' in zz
     and 'napServer.get("/api/v1/hra?id=" + id)' in zz and 'throw new IOException("nesparovano")' in zz,
     'S5 zaloha N&P: seznam her -> Sonic / BIOS -> podepsane stazeni /api/v1/hra')
over('AtarihelpTls.pouzij(c, log != null ? NapStahovaniSeSouhlasem::loguj : null);' in ST
     and MA.count('AtarihelpTls.pouzij(bc, this::appendNativeLog);') == 1 and MA.count('AtarihelpTls.pouzij(pc, this::appendNativeLog);') == 1,
     'S6 HTTPS atarihelp.eu (Sonic, BIOS, BIOS pro PS1) s doplnenim certifikatu / poznanim mistniho serveru (B298 / B299)')
over('static String duvod(String v, String mistni, boolean en)' in ST and 'prelozVysledek(zprava, NapJazyk.en(a), AtarihelpTls.mistniServer(' in ST
     and 'popisSlozky(a, korenSlozky)' in ST, 'S7 okno Nestazeno: duvod u kazdeho souboru a skutecna slozka')
pb = blok(MA, '    private void ps1EnsureBios(File sysDir) {', '\n    }\n')
over('NapStahovaniSeSouhlasem.dalsiKoren' in pb and 'NapStahovaniSeSouhlasem.dalsiKoren = napSlozkaAppky();' in MA
     and 'for (File k : koreny(korenSlozky))' in ST,
     'S8 Sonic a BIOS se hledaji i ve slozce appky (Sega, PS1, kontrola pri startu)')
over("if (h && typeof h.soubor === 'string' && /bios/i.test(h.soubor)) continue;" in HR, 'S9 N&P HRY: BIOS ze serveru neni v seznamu her')
dg = blok(MA, '    private void napDiagnostikaUloziste() {', '\n    }\n')
over('napDiagnostikaUloziste();' in MA and 'B337 ULOZISTE pri startu' in dg and 'NapStahovaniSeSouhlasem.log = this::appendNativeLog;' in dg,
     'S10 log pri startu: povoleni uloziste, kam se uklada, Sonic / BIOS; stahovani do logu appky')

# ---- A) Atari na TV ----
tm = blok(MA, '    private void atariCppTvMenu() {', '\n    }\n')
prvni = re.search(r'final String\[\] polozky = \{\s*tx\("([^"]+)"', tm)
over(prvni is not None and 'HLAVNÍ MENU' in prvni.group(1) and 'case 0: atariTvDoMenu("polozka HLAVNI MENU"); break;' in tm
     and 'case 1: atariZarizeniAkce(1); break;' in tm and 'default: atariZarizeniAkce(7); break;' in tm,
     'A1 menu pristroje Atari na TV: prvni polozka HLAVNI MENU (ramecek na ni), pak NET / HRY ... NAPOVEDA')
over('d.setOnKeyListener((dlg, kod, e) -> {' in tm and 'kod != KeyEvent.KEYCODE_BACK' in tm and 'atariTvDoMenu("ZPET 2x");' in tm
     and 'ZPĚT znovu = hlavní menu' in tm, 'A2 ZPET v otevrenem menu = hlavni menu (ZPET 2x), napis v titulku')

# ---- L) log z TV, napoveda, verze ----
over('<small class="stTvLogPopis">LOG APPKY PRO PODPORU</small><i id="stTvLog" data-nepreklad></i>' in IX
     and "lg.textContent=adr.replace(/\\/+$/,'')+'/log';" in IX and "'LOG APPKY PRO PODPORU': 'APP LOG FOR SUPPORT'" in EN,
     'L1 OPTIONS: pod adresou WEB TV i adresa logu (CZ / EN)')
over('<b>Log z televize:</b>' in NCZ and '<b>Log from the TV:</b>' in NEN and 'dvakrát za sebou' in NCZ and 'twice in a row' in NEN
     and '<b>Sonic a BIOS v televizi:</b>' in NCZ and '<b>Sonic and BIOS on the TV:</b>' in NEN,
     'L2 napoveda CZ / EN: Atari ZPET 2x, zamcena karta a server, Sonic + BIOS na TV, log z TV')
over('versionCode 380' in GR and 'versionName "EMU10-B337-TV-SEGA-PS1-SOUBORY"' in GR, 'L3 build.gradle: versionCode 380 "EMU10-B337-TV-SEGA-PS1-SOUBORY"')

# ES5 (starsi WebView v televizich): zmenene skripty stranek
for jm, txt in (('hry/odemknout.html', OD), ('spolecne/nap_tv_ovladani.js', TV)):
    js = txt
    if jm.endswith('.html'):
        js = '\n'.join(re.findall(r'<script>([\s\S]*?)</script>', txt))
    r = subprocess.run(['node', '-e', "const a=require('/opt/npm-tools/node_modules/acorn');a.parse(process.argv[1],{ecmaVersion:5});console.log('ok')", js],
                       capture_output=True, text=True, timeout=60)
    over(r.stdout.strip() == 'ok', 'E %s je ES5%s' % (jm, '' if r.stdout.strip() == 'ok' else ': ' + r.stderr[-200:]))

vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (sum(1 for v in vystup if v.startswith(('OK', 'CHYBA'))), chyb))
print('\n'.join(vystup))
open(os.path.join(ZDE, 'VYSTUP_kontrola_b337.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
