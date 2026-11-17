#!/usr/bin/env python3
"""B342: kontrola zapojeni - OBRAZOVKA NA VYSKU / NA SIRKU (telefon se otoci, TV pruh 9:16 uprostred), tlacitko
otoceni v hlavnim menu (C++), v pristroji Atari (C++), na strankach Sega a PS1, v menu konzole na TV a radek
OBRAZOVKA v OPTIONS.
Pouziti: python3 test_overeni/b342/kontrola_b342.py   -> VYSTUP_kontrola_b342.txt
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
NO = cti('app/src/main/java/eu/atarihelp/emu10/NapObrazovka.java')
OB = cti('app/src/main/java/eu/atarihelp/emu10/NapObal.java')
NB = cti('app/src/main/java/eu/atarihelp/emu10/NativeMenuBridge.java')
KA = cti('app/src/main/cpp/menu/nap_menu_karusel.h')
KR = cti('app/src/main/cpp/menu/nap_menu_kresba.h')
GL = cti('app/src/main/cpp/menu/nap_menu_gl.cpp')
SH = cti('app/src/main/cpp/menu/nap_menu_shadery.h')
DV = cti('app/src/main/cpp/atari/nap_atari_device.h')
IX = cti('app/src/main/assets/index.html')
SE = cti('app/src/main/assets/emu_sega/index.html')
P1 = cti('app/src/main/assets/emu_ps1/index.html')
JS = cti('app/src/main/assets/spolecne/nap_obrazovka.js')
EM = cti('app/src/main/assets/jazyk/en_menu.js')
EK = cti('app/src/main/assets/jazyk/en_konzole.js')
NC = cti('app/src/main/assets/napoveda/index.html')
NE = cti('app/src/main/assets/napoveda/en.html')
GR = cti('app/build.gradle')
MF = cti('app/src/main/AndroidManifest.xml')
vystup, chyb = [], 0


def over(ok, co):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)


def blok(text, zac, kon):
    i = text.index(zac)
    return text[i:text.index(kon, i + len(zac))]


# ---- J) Java ----
oc = blok(MA, '        rootFrame = new FrameLayout(this);', 'if (!NapJazyk.obrazovkaHotova(this))')
over('napObal = new NapObal(this);' in oc and 'napObal.addView(rootFrame, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));' in oc
     and 'setContentView(napObal);' in oc and 'setContentView(rootFrame);' not in MA
     and oc.index('setContentView(napObal);') < oc.index('napObrazPouzij(NapObrazovka.MENU, "start appky");'),
     'J1 obsah okna = cerny obal NapObal, v nem rootFrame (vsechny plochy C++ i stranka); hned volba hlavniho menu')
over('protected void onMeasure(int ws, int hs)' in OB and 'NapObrazovka.sirkaPruhu(W, H)' in OB and 'c.layout(x, 0, x + w, H);' in OB
     and 'final int x = (W - w) / 2;' in OB and 'setBackgroundColor(Color.BLACK);' in OB,
     'J2 NapObal: pruh = deti meri na sirku 9:16 a klade na stred, po stranach cerno (jinak cela obrazovka)')
ps = blok(MA, '            public void onPageStarted(WebView v, String url, Bitmap favicon) {', 'public boolean shouldOverrideUrlLoading')
over('napObrazPouzij(NapObrazovka.skupina(url), "stranka " + compactUrl(url));' in ps
     and ps.index('atariZarizeniPodleUrl(url);') < ps.index('napObrazPouzij(NapObrazovka.skupina(url)'),
     'J3 onPageStarted: kazda stranka pouzije volbu sve skupiny (menu / Atari / Sega / PS1)')
pz = blok(MA, '    void napObrazPouzij(int sk, String proc) {', '\n    }\n')
over('if (napJeTv()) {' in pz and 'NapObrazovka.pruh(st, true)' in pz and 'napObal.pruh(p)' in pz and pz.index('return;') < pz.index('setRequestedOrientation(o);')
     and 'NapObrazovka.orientace(st)' in pz and 'if (o != napObrazPozadavek)' in pz and 'napObal.pruh(false);' in pz,
     'J4 napObrazPouzij: TV = jen pruh (zadne setRequestedOrientation - TV by se mohla otocit bokem), telefon = setRequestedOrientation jen pri zmene')
pr = blok(MA, '    void napObrazPrepni(String odkud) {', '\n    }\n')
over('NapObrazovka.prepni(naSirku)' in pr and 'putInt(NapObrazovka.klic(sk), novy)' in pr and 'napObrazPouzij(sk, odkud);' in pr
     and '"B342 OBRAZOVKA PREPNUTA ("' in pr and 'napObrazToast(sk, novy);' in pr,
     'J5 napObrazPrepni: opak toho, co je na obrazovce, ulozit pro skupinu, pouzit, log a kratke potvrzeni')
ns = blok(MA, '    void napObrazNastav(String co) {', '\n    }\n')
over('"auto".equals(c)' in ns and 'ed.remove(NapObrazovka.klic(i))' in ns and 'NapObrazovka.klic(NapObrazovka.MENU)' in ns,
     'J6 OPTIONS: NA VYSKU / NA SIRKU = hlavni menu, PODLE TELEFONU = vsechny skupiny zpet (AUTO)')
ah = blok(MA, '    public class AHObrazovka {', '\n    }\n\n')
over('web.addJavascriptInterface(new AHObrazovka(), "AHOBRAZ");' in MA and 'u.startsWith("file:///android_asset/")' in ah
     and ah.count('@JavascriptInterface') == 3 and 'ui.post(() -> napObrazPrepni("tlacitko na strance"));' in ah and 'ui.post(() -> napObrazNastav(co));' in ah,
     'J7 most AHOBRAZ (stav / prepni / nastav) - jen stranky appky, zmeny na UI vlakne')
ma = blok(MA, '    private void menuAkce(int kod, String odkud) {', '\n    }\n')
over('kod == NativeMenuBridge.AKCE_OTOCIT' in ma and 'napObrazPrepni("hlavni menu " + odkud);' in ma
     and 'public static final int AKCE_OTOCIT = 2001;' in NB and 'AKCE_OTOCIT = 2001,' in KA,
     'J8 hlavni menu: AKCE_OTOCIT 2001 (C++ i Java) -> napObrazPrepni')
az = blok(MA, '    private void atariZarizeniAkce(int kod) {', '\n    }\n')
over('case 12:' in az and 'napObrazPrepni("pristroj Atari");' in az and '"OTOCIT_OBRAZOVKU"' in az and 'SVC_OTOC = 12,' in DV,
     'J9 pristroj Atari: servisni akce 12 (SVC_OTOC) -> napObrazPrepni')
km = blok(MA, '    private void konzoleTvMenu(final boolean ps1) {', '\n    }\n')
over('final String otoc = NapObrazovka.tlacitko(napObrazNaSirku(), NapJazyk.en(this));' in km
     and km.count('otoc,') == 2 and 'new int[]{0, 4, 5, 6, 1, 2, 3, 11, 9, -1}' in km and 'new int[]{0, 5, 7, 8, 10, 1, 2, 11, 9, -1}' in km,
     'J10 TV menu konzole (ZPET) PS1 / SEGA: polozka OBRAZOVKA NA VYSKU / NA SIRKU (akce 11) - pocet polozek = pocet akci')
ka = blok(MA, '    private void konzoleTvAkce(boolean ps1, int akce) {', '\n    }\n')
over('case 11:' in ka and 'napObrazPrepni("menu konzole " + jm);' in ka, 'J11 konzoleTvAkce 11 -> napObrazPrepni')
at = blok(MA, '    private void atariCppTvMenu() {', '\n    }\n')
pol = blok(at, 'final String[] polozky = {', '};')
n_pol = len(re.findall(r'^\s+(?:tx\(|NapObrazovka\.tlacitko|")', pol, re.M))
over('NapObrazovka.tlacitko(napObrazNaSirku(), NapJazyk.en(this)),' in at and 'case 10: atariZarizeniAkce(12); break;' in at
     and 'case 11: napTvOvladaniDialog(OVL_ATARI); break;' in at and n_pol == 13,
     'J12 TV menu Atari: OBRAZOVKA NA VYSKU / NA SIRKU (10) pred OVLADANIM (11), NAPOVEDA posledni (13 polozek: %d)' % n_pol)
it = blok(MA, '        @JavascriptInterface public String spustPs1() {', 'BUILD2SA35')
over('rootFrame.getWidth() > 0 ? rootFrame.getWidth() : dm.widthPixels' in it and 'ps1PlochaUmisti(0, 0, pw, ph, pw > ph);' in it,
     'J13 intro PS1: plocha podle velikosti appky (rootFrame), ne displeje - na TV muze byt pruh')
over('android:screenOrientation' not in MF and 'orientation|screenSize' in MF,
     'J14 manifest: zadna pevna orientace, otoceni bez restartu aktivity (configChanges) - rozhoduje NapObrazovka')
over('SCREEN_ORIENTATION_PORTRAIT' in NO and 'SCREEN_ORIENTATION_SENSOR_LANDSCAPE' in NO and 'SCREEN_ORIENTATION_UNSPECIFIED' in NO
     and 'PRUH_SIRKA = 9, PRUH_VYSKA = 16' in NO,
     'J15 NapObrazovka: telefon PORTRAIT / SENSOR_LANDSCAPE / UNSPECIFIED (AUTO), TV pruh 9:16')

# ---- C) C++ ----
kl = blok(KA, '  int klepnuti(float x, float y) {', '    const int i = kartaNa(x, y);')
over('return AKCE_OTOCIT;' in kl and kl.index('return AKCE_NASTAVENI;') < kl.index('return AKCE_OTOCIT;'),
     'C1 karusel: klepnuti na otoceni (pred kartami) = AKCE_OTOCIT; kolecko beze zmeny')
over(KA.count('r.otocR = r.ozubR;') == 2 and 'r.otocX = r.ozubR * 1.9f;' in KA and 'r.otocX = r.ozubX - r.ozubR * 2.6f;' in KA,
     'C2 rozlozeni: na vysku zrcadlo kolecka vlevo, na sirku vlevo (u skoro ctverce vedle kolecka)')
over('ui(c, 9, r.otocX - R, r.otocY - R' in KR and 'if (!c.tv && r.otocR > 0.f)' in KR and 'double tOtoc = -1;' in KR
     and 'g_efekty.tOtoc = t;' in GL and 'g_efekty.tOtoc = -1;' in GL,
     'C3 kresba: ikona typ 9 (jen na displeji, ne v obrazu pro TV / PC), animace po klepnuti')
over('} else if (uTyp == 9) {' in SH and 'float sipka(vec2 p, float r)' in SH and 'float telefon(vec2 p, float r, float a)' in SH,
     'C4 shader FS_UI typ 9: telefon + dve zahnute sipky (SDF)')
over('ID_OTOC = 600,' in DV and 'if (otocVyskaHit(x, y)) return ID_OTOC;' in DV and 'otocVyskaRR(o)) drawOtocVyska(c, o);' in DV
     and 'if (otocHit(x, y)) { otocPid = pid; mark(otocRect()); return 0; }' in DV and 'if (otocHit(x, y) && svc) *svc = SVC_OTOC;' in DV
     and 'spriteBlit(c, spOtoc[otocPid >= 0 ? 1 : 0])' in DV,
     'C5 pristroj Atari: na vysku pilulka NA SIRKU ve stavovem radku, na sirku pilulka vedle kolecka -> SVC_OTOC')

# ---- S) stranky ----
over('<script src="../spolecne/nap_obrazovka.js"></script>' in SE and 'id="btnSegaOtoc" class="napOtoc" type="button" data-nap-otoc' in SE
     and '#segaLayoutEditBar.show ~ #btnSegaOtoc{display:none!important;}' in SE
     and SE.index('id="segaLayoutEditBar"') < SE.index('id="btnSegaOtoc"'),
     'S1 Sega: tlacitko otoceni (za listou uprav - pri uprave rozlozeni schovane), skript nap_obrazovka.js')
over('<script src="../spolecne/nap_obrazovka.js"></script>' in P1 and 'id="btnPs1Otoc" class="napOtoc" type="button" data-nap-otoc' in P1
     and '#layoutEditBar.show ~ #btnPs1Otoc{display:none!important;}' in P1 and P1.index('id="layoutEditBar"') < P1.index('id="btnPs1Otoc"'),
     'S2 PS1: tlacitko otoceni (za listou uprav), skript nap_obrazovka.js')
over('W.AHOBRAZ.prepni()' in JS and "if (!W.AHOBRAZ) { b.style.display = 'none'; return; }" in JS and "tel.setAttribute('transform', 'rotate(-90)')" in JS
     and "W.addEventListener('resize', obnov);" in JS and 'ted - nedavno < 700' in JS,
     'S3 nap_obrazovka.js: klik -> AHOBRAZ.prepni (dvojklik = jednou), ikona a popisek podle otoceni, bez mostu schovane')
over('id="stObraz"' in IX and 'data-obraz="vyska"' in IX and 'data-obraz="sirka"' in IX and 'data-obraz="auto"' in IX
     and "AHOBRAZ.nastav(b.getAttribute('data-obraz'))" in IX and 'try{ obrazObnov(); }catch(e){}' in IX
     and IX.index('id="stJazyk"') < IX.index('id="stObraz"') < IX.index('id="stStartTv"'),
     'S4 OPTIONS: radek OBRAZOVKA hned pod jazykem (NA VYSKU / NA SIRKU / PODLE TELEFONU), obnovi se pri otevreni')
over("'OBRAZOVKA': 'SCREEN'" in EM and "'NA VÝŠKU': 'PORTRAIT'" in EM and "'NA ŠÍŘKU': 'LANDSCAPE'" in EM and "'PODLE TELEFONU': 'AUTO-ROTATE'" in EM
     and "'OBRAZOVKA NA ŠÍŘKU': 'SCREEN TO LANDSCAPE'" in EK and "'OBRAZOVKA NA VÝŠKU': 'SCREEN TO PORTRAIT'" in EK,
     'S5 anglicky: OPTIONS (en_menu.js) a popisek tlacitka (en_konzole.js)')
js = [JS] + ['\n'.join(re.findall(r'<script>([\s\S]*?)</script>', t)) for t in (IX, SE, P1)]
ok_es5 = True
for kod in js:
    r = subprocess.run(['node', '-e', "const a=require('/opt/npm-tools/node_modules/acorn');a.parse(process.argv[1],{ecmaVersion:5});console.log('ok')", kod],
                       capture_output=True, text=True, timeout=60)
    ok_es5 &= r.stdout.strip() == 'ok'
over(ok_es5, 'S6 nap_obrazovka.js a skripty index.html / Sega / PS1 jsou ES5')

# ---- N) napoveda a verze ----
over('id="obrazovka"' in NC and 'id="obrazovka"' in NE and NC.count('data-skok="obrazovka"') >= 4 and NE.count('data-skok="obrazovka"') >= 4
     and 'pruhu uprostřed' in NC and 'strip in the middle' in NE,
     'N1 napoveda CZ / EN: Obrazovka na vysku a na sirku (+ odkazy z menu, Atari, Segy, PS1 a TV)')
over('versionCode 385' in GR and 'versionName "EMU10-B342-OBRAZOVKA-SIRKA-VYSKA"' in GR, 'N2 build.gradle: versionCode 385 "EMU10-B342-OBRAZOVKA-SIRKA-VYSKA"')

vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (sum(1 for v in vystup if v.startswith(('OK', 'CHYBA'))), chyb))
print('\n'.join(vystup))
open(os.path.join(ZDE, 'VYSTUP_kontrola_b342.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
