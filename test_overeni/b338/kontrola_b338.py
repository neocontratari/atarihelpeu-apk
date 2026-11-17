#!/usr/bin/env python3
"""B338: kontrola zapojeni (Android TV: menu konzole SEGA / PS1 na ZPET, VYKON ZARIZENI AUTO / LOW / HIGH).
Pouziti: python3 test_overeni/b338/kontrola_b338.py   -> VYSTUP_kontrola_b338.txt
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
PR = cti('app/src/main/java/eu/atarihelp/emu10/NapPravidla.java')
SG = cti('app/src/main/cpp/nap_sega_native_proof.cpp')
GR = cti('app/build.gradle')
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


# ---- K) menu konzole SEGA / PS1 na TV ----
bp = blok(MA, '    public void onBackPressed() {', '\n    }\n')
i_at = bp.index('atariCppTvMenu();')
i_k = bp.index('konzoleTvMenu(isPs1OwnerUrl(web.getUrl()));')
i_m = bp.index('if (menuPlocha != null) { menuDoPozadi(); return; }')
over(i_at < i_k < i_m and 'napJeTv() && web != null && (isSegaNativeOwnerUrl(web.getUrl()) || isPs1OwnerUrl(web.getUrl()))' in bp,
     'K1 ZPET na TV ve SEGA / PS1 -> menu konzole (po Atari, pred menu / historii); telefon beze zmeny')
km = blok(MA, '    private void konzoleTvMenu(final boolean ps1) {', '\n    }\n')
over('tx("HRY ZE SERVERU N&P", "GAMES FROM THE N&P SERVER")' in km and 'tx("RESET (BIOS BEZ CD)", "RESET (BIOS WITHOUT CD)")' in km
     and km.count('tx("⌂ HLAVNÍ MENU", "⌂ MAIN MENU")') == 2 and 'w == polozky.length - 1 ? -1 : w' in km
     and 'kod != KeyEvent.KEYCODE_BACK' in km and 'konzoleTvAkce(ps1, -2);' in km and 'ZPĚT znovu = hlavní menu' in km,
     'K2 menu konzole: hry ze serveru N&P, ulozit / nacist pozici, PS1 reset, HLAVNI MENU (posledni), ZPET 2x = hlavni menu')
ka = blok(MA, '    private void konzoleTvAkce(boolean ps1, int akce) {', '\n    }\n')
over('napOtevriHry(ps1 ? "ps1" : "sega");' in ka and 'napTvKlik(ps1 ? "btnSave" : "segaBtnSaveState")' in ka
     and 'napTvKlik(ps1 ? "btnQuickLoad" : "segaBtnLoadState")' in ka and 'napTvKlik("btnReset")' in ka
     and 'web.loadUrl("file:///android_asset/index.html");' in ka,
     'K3 akce: N&P HRY (sega / ps1), tlacitka stranky pres click() (i schovana na sirku), hlavni menu (jadro stoji v onPageStarted)')
over("static String tvKlik(String id)" in PR and "if(b&&!b.disabled){b.click();return 'ok';}return 'chybi';" in PR
     and 'return NapPravidla.tvKlik(id);' in MA, 'K4 NapPravidla.tvKlik - JavaScript klepnuti (test v Chromiu na skutecnych strankach)')
ts = blok(MA, '    private void napTvStrankaNactena(String url) {', '\n    }\n')
over('isSegaNativeOwnerUrl(url) || isPs1OwnerUrl(url)' in ts and 'Ovladač: ZPĚT = menu konzole' in ts,
     'K5 vstup do SEGA / PS1 na TV -> kratka rada (ZPET = menu konzole)')

# ---- V) vykon zarizeni ----
over('static boolean jeSlabeZarizeni(boolean bit64, int jader, int maxMhz)' in PR
     and 'return !bit64 || (maxMhz > 0 && maxMhz < 1500) || (jader > 0 && jader <= 2);' in PR
     and 'NapPravidla.jeSlabeZarizeni(bit64, jader, mhz)' in MA and 'cpufreq/cpuinfo_max_freq' in MA,
     'V1 AUTO: slabe = jen 32 bitu, takt pod 1,5 GHz nebo 2 jadra (takt z /sys)')
vl = blok(MA, '    boolean napVykonLow() {', '\n    }\n')
over('return r == 1 || (r == 0 && napSlabeZarizeni());' in vl and 'getInt("rezim", 0)' in MA,
     'V2 LOW = volba LOW, nebo AUTO na slabem zarizeni (volba v SharedPreferences nap_vykon)')
sp = blok(MA, '        public String setPerformanceMode(String mode) {', '\n        }\n')
over('if (napVykonLow() && !"LOW".equals(m)) {' in sp and sp.index('if (napVykonLow()') < sp.index('nativePerformanceMode = m;'),
     'V3 Sega: LOW zarizeni / volba vynuti nativni LOW (snimani kazdy 2. snimek, kresleni 30/s)')
pv = blok(MA, '        void plochaPodleVykonu(String proc) {', '\n        }\n')
over('st.setDefaultBufferSize(SRC_W, SRC_H);' in pv and 'setOpaque(false);' in pv and 'setTransform(m);' in pv
     and 'napSegaVyrez(w, h)' in pv and 'setTransform(null);' in pv,
     'V4 Sega LOW: plocha TextureView 320x224, zvetsuje grafika (setTransform s pomerem stran), HIGH zpet na plnou velikost')
dt = blok(MA, '        private void drawTextureFrame(Canvas canvas) {', '\n        }\n')
over('if (malaPlocha && canvas.getWidth() == SRC_W && canvas.getHeight() == SRC_H) {' in dt and 'canvas.drawBitmap(bitmap, 0f, 0f, null);' in dt,
     'V5 Sega LOW: kresleni 1:1 jen kdyz plocha opravdu 320x224 (jinak po staru - zadne riziko)')
over(MA.count('plochaPodleVykonu("plocha pripravena");') == 1 and MA.count('plochaPodleVykonu("zmena velikosti");') == 1,
     'V6 plocha podle vykonu pri pripraveni i zmene velikosti (TextureView ji pri zmene vraci na velikost View)')
gl = blok(SG, 'static void nap_sega_thread_fn(', '\n}\n')
over('if (g_native_perf_low.load()) {' in gl and 'glClear(GL_COLOR_BUFFER_BIT);' in gl and 'usleep(50000);' in gl,
     'V7 C++: druha (neviditelna) cesta OpenGL v LOW jednou zcerna a stoji')
st = blok(MA, '                napOverStav("start");', 'final boolean demo = ')
over('if (!napIntroUkazano && napVykonLow()) {' in st and 'napIntroUkazano = true;' in st,
     'V8 LOW: start appky bez uvodniho dema (TV: demo se do 7 s nepripravilo) i bez stareho intra')
ar = blok(MA, '        public String vykon() {', '\n        }\n')
an = blok(MA, '        public String nastavVykon(String r) {', '\n        }\n')
over('"{\\"rezim\\":\\"' in ar and 'jsonQuote(popis)' in ar and 'putInt("rezim", v)' in an and 'return vykon();' in an,
     'V9 most AHRENDER.vykon / nastavVykon (JSON: rezim, low, slabe, veta v jazyce appky)')

# ---- O) OPTIONS, napoveda, verze ----
over('<div class="stlabel2">VÝKON ZAŘÍZENÍ</div>' in IX and 'data-vykon="a">AUTO' in IX and 'data-vykon="l">LOW' in IX
     and 'data-vykon="h">HIGH' in IX and '<div class="sthint" id="stVykonHint" data-nepreklad></div>' in IX
     and "AHRENDER.nastavVykon(b.getAttribute('data-vykon'))" in IX and "'VÝKON ZAŘÍZENÍ': 'DEVICE PERFORMANCE'" in EN,
     'O1 OPTIONS: VYKON ZARIZENI AUTO / LOW / HIGH + veta z Javy (CZ / EN)')
over('<b>SEGA a PS1 na TV:</b>' in NCZ and '<b>SEGA and PS1 on the TV:</b>' in NEN and '<b>Slabá televize:</b>' in NCZ
     and '<b>A weak TV:</b>' in NEN, 'O2 napoveda CZ / EN: menu konzole na TV, VYKON ZARIZENI / LOW')
over('versionCode 381' in GR and 'versionName "EMU10-B338-TV-MENU-KONZOLE-LOW"' in GR, 'O3 build.gradle: versionCode 381 "EMU10-B338-TV-MENU-KONZOLE-LOW"')

# ES5 skript OPTIONS (stranka menu bezi i ve starsich WebView)
js = '\n'.join(re.findall(r'<script>([\s\S]*?)</script>', IX))
r = subprocess.run(['node', '-e', "const a=require('/opt/npm-tools/node_modules/acorn');a.parse(process.argv[1],{ecmaVersion:5});console.log('ok')", js],
                   capture_output=True, text=True, timeout=60)
over(r.stdout.strip() == 'ok', 'E skripty index.html jsou ES5%s' % ('' if r.stdout.strip() == 'ok' else ': ' + r.stderr[-200:]))

vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (sum(1 for v in vystup if v.startswith(('OK', 'CHYBA'))), chyb))
print('\n'.join(vystup))
open(os.path.join(ZDE, 'VYSTUP_kontrola_b338.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
