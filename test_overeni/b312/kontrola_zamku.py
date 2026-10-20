#!/usr/bin/env python3
# B312: kontrola zdrojaku MainActivity.java - ZAMKY SEGA / PS1 PLATI VSUDE a SERVIS.
# Activity na PC spustit nejde (Android), proto se kontroluje primo kod:
#  1) kazda cesta, ktera spousti hru Segy / PS1 (stazeni z webu, ZIP s ROM, AHPS1.ps1DownloadAndBoot,
#     vyber hry PS1 z telefonu), se NEJDRIV zepta konzoleZamcena(...) - driv nez cokoli stahne,
#     spusti vlakno nebo otevre stranku
#  2) kazde loadUrl(SEGA_URL / PS1_URL) je az za takovou branou
#  3) hlavni ramec: shouldInterceptRequest zkontroluje zamek jako PRVNI (stranka zamcene konzole se
#     vubec nenacte), onPageStarted totez jako pojistka (historie stranek)
#  4) vsechny cesty z webu (odkaz, stazeni, ahgame://, AHNET.runGameUrl) vedou jen na hlidane metody
#  5) SERVIS: odemkniDalsi / zamkniVse (AHINTRO) bez NapServis.otevreno nic neudelaji
# Rozhodovani samo (ktera stranka je ktera konzole, duvod) testuje TestB312Java v JVM.
#   python3 kontrola_zamku.py      -> VYSTUP_kontrola_zamku.txt
import os
import re
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.normpath(os.path.join(ZDE, '../../app/src/main/java/eu/atarihelp/emu10/MainActivity.java'))
t = open(SRC, encoding='utf-8').read()
vystup = []
kontrol = chyb = 0


def over(ok, co):
    global kontrol, chyb
    kontrol += 1
    if not ok:
        chyb += 1
    r = ('OK     ' if ok else 'CHYBA  ') + co
    print(r)
    vystup.append(r)


def telo(podpis_re):
    """Telo metody (od { za podpisem po odpovidajici }), bez komentaru a retezcu se nepocita - staci zavorky."""
    m = re.search(podpis_re, t)
    if not m:
        return None, -1
    i = t.index('{', m.end() - 1)
    hl = 0
    for j in range(i, len(t)):
        if t[j] == '{':
            hl += 1
        elif t[j] == '}':
            hl -= 1
            if hl == 0:
                return t[i:j + 1], m.start()
    return None, -1


NEBEZPECNE = ['new Thread', 'loadUrl(', 'fetchUrlBytes(', 'startActivityForResult(', 'pendingSegaGame =',
              'startPs1RemoteDownloadAndBoot(', 'injectPendingSegaGame', 'ps1RemoteDownloadActive = true']
BRANY = [
    (r'private void downloadAndRunSega\(final String url\)', 'SEGA', 'stazeni hry Segy z webu'),
    (r'private void downloadAndRunSegaArchive\(final String url\)', 'SEGA', 'stazeni ZIP se hrou Segy z webu'),
    (r'private void openSegaRomBytes\(byte\[\] data, String name, String reason\)', 'SEGA', 'otevreni ROM Segy (i ZIP ze stazeni hry Atari)'),
    (r'private void downloadAndRunPs1Remote\(final String url, final String reason\)', 'PS1', 'stazeni hry PS1 z webu'),
    (r'private void startPs1RemoteDownloadAndBoot\(final String rawUrl\)', 'PS1', 'AHPS1.ps1DownloadAndBoot (zavola i web)'),
    (r'public void ps1PickGame\(\)', 'PS1', 'AHPS1.ps1PickGame - vyber hry PS1 z telefonu'),
]
hlidane = []
for podpis, konzole, popis in BRANY:
    b, _ = telo(podpis)
    jm = re.search(r'(\w+)\\\(', podpis).group(1)
    hlidane.append(jm)
    if b is None:
        over(False, '1) %s: metoda nenalezena' % jm)
        continue
    brana = b.find('konzoleZamcena(NapPravidla.%s' % konzole)
    prvni = min([b.find(x) for x in NEBEZPECNE if b.find(x) >= 0] or [len(b)])
    over(brana >= 0 and brana < prvni,
         '1) %s (%s): zamek %s se zkontroluje driv nez cokoli dalsiho (%s)' % (jm, popis, konzole,
                                                                              'brana na znaku %d, prvni akce %d' % (brana, prvni)))

# 2) kazde loadUrl(SEGA_URL / PS1_URL) uvnitr hlidane metody
mista = [m.start() for m in re.finditer(r'loadUrl\((SEGA_URL|PS1_URL)\)', t)]
spatne = []
for pos in mista:
    # nejblizsi predchozi deklarace metody
    dekl = [m for m in re.finditer(r'\n    (?:private|public|protected)[^\n;=]*?\s(\w+)\([^)]*\)\s*(?:throws [\w., ]+)?\{', t) if m.start() < pos]
    jm = dekl[-1].group(1) if dekl else '?'
    if jm not in hlidane:
        spatne.append(jm)
over(len(mista) >= 2 and not spatne, '2) loadUrl(SEGA_URL / PS1_URL): %d mist, vsechna v hlidanych metodach%s' % (len(mista), (' - MIMO: ' + ', '.join(spatne)) if spatne else ''))

# 3) hlavni ramec
b, _ = telo(r'public WebResourceResponse shouldInterceptRequest\(WebView v, WebResourceRequest request\)')
ok3 = False
if b:
    i_main = b.find('request.isForMainFrame()')
    i_z = b.find('zamcenaStrankaOdpoved(reqUrl)')
    i_dalsi = min([x for x in (b.find('interceptAhGameBridgeNavigation(reqUrl)'), b.find('interceptMainFrameGameNavigation(reqUrl)')) if x >= 0] or [len(b)])
    ok3 = 0 <= i_main < i_z < i_dalsi
over(ok3, '3) shouldInterceptRequest: pro hlavni ramec je zamek PRVNI (zamcena stranka emulatoru se vubec nenacte)')
z, _ = telo(r'private WebResourceResponse zamcenaStrankaOdpoved\(String url\)')
over(z is not None and 'NapPravidla.konzoleStranky(url)' in z and 'konzoleZamcena(' in z and "location.replace('file:///android_asset/index.html')" in z,
     '3) zamcenaStrankaOdpoved: misto stranky emulatoru kratka stranka, ktera skoci do hlavniho menu')
b, _ = telo(r'public void onPageStarted\(WebView v, String url, Bitmap favicon\)')
pj, _ = telo(r'private boolean zamcenaStrankaPojistka\(String url\)')
over(b is not None and 'zamcenaStrankaPojistka(url)' in b and 'v.stopLoading()' in b
     and 'web.loadUrl("file:///android_asset/index.html")' in b and pj is not None
     and 'NapPravidla.konzoleStranky(url)' in pj and 'konzoleZamcena(' in pj and 'zamekStrankaUrl' in pj,
     '3) onPageStarted: pojistka (i stranka z historie) - zastavit a do hlavniho menu; co uz vyridil shouldInterceptRequest, se podruhe nedela')
k, _ = telo(r'private boolean konzoleZamcena\(String konzole, String odkud, String co\)')
over(k is not None and 'NapPostup.segaOdemcena(this)' in k and 'NapPostup.ps1Odemcena(this)' in k and 'NapPravidla.zamceno(' in k
     and 'NapPravidla.duvodZamku(' in k and 'ukazZamek(' in k and 'appendNativeLog("B312 ZAMEK ' in k,
     '3) konzoleZamcena: stav z NapPostup, rozhodnuti NapPravidla, duvod cloveku (Toast) a radek B312 ZAMEK do logu')

# 4) cesty z webu vedou jen na hlidane metody
CESTY = [(r'private void routeGameDownloadUrl\(String url, String source\)', 'routeGameDownloadUrl (AHNET.runGameUrl, ahgame://)'),
         (r'private boolean handleMaybeGameUrl\(String url\)', 'handleMaybeGameUrl (odkaz na webu)')]
for podpis, popis in CESTY:
    b, _ = telo(podpis)
    volani = set(re.findall(r'\b(downloadAndRun\w*|openSegaRomBytes|startPs1RemoteDownloadAndBoot)\(', b or ''))
    nehlidane = sorted(v for v in volani if v not in hlidane and v != 'downloadAndRun')
    over(b is not None and volani and not nehlidane, '4) %s vola jen: %s' % (popis, ', '.join(sorted(volani))))
i = t.find('web.setDownloadListener(')
dl = t[i:t.find('});', i)] if i >= 0 else ''
volani = set(re.findall(r'\b(downloadAndRun\w*|openSegaRomBytes|startPs1RemoteDownloadAndBoot)\(', dl))
over(dl and volani and all(v in hlidane or v == 'downloadAndRun' for v in volani),
     '4) DownloadListener (stazeni souboru z webu) vola jen: %s' % ', '.join(sorted(volani)))
i = t.find('ZIP_CONTAINS_SEGA')
usek = t[i:t.find('return;', i)] if i >= 0 else ''
over('openSegaRomBytes(sega.data' in usek, '4) stazeni hry Atari / ZIP (downloadAndRun): kdyz je v ZIP hra Segy, jde pres hlidany openSegaRomBytes')

# 5) servis
for jm, akce in (('odemkniDalsi', 'NapPostup.odemkniDalsi('), ('zamkniVse', 'NapPostup.zamkniVse(')):
    b, _ = telo(r'@JavascriptInterface public String %s\(\)' % jm)
    ok = b is not None and b.find('NapServis.otevreno(') >= 0 and b.find('return "ZAMCENO"') >= 0 \
        and b.find('NapServis.otevreno(') < b.find(akce)
    over(ok, '5) AHINTRO.%s: bez otevreneho servisu vrati ZAMCENO a %s se nezavola' % (jm, akce.rstrip('(')))
b, _ = telo(r'@JavascriptInterface public String servisKod\(String kod\)')
over(b is not None and 'NapServis.zkus(kod' in b and not re.search(r'appendNativeLog\([^;]*\bkod\b(?!\s*spravny|\s*odmitnut)', b),
     '5) AHINTRO.servisKod: overi NapServis, do logu jen vysledek (napsany kod ne)')

vysl = 'VYSLEDEK: %d kontrol, %d CHYBA' % (kontrol, chyb)
print(vysl)
vystup.append(vysl)
open(os.path.join(ZDE, 'VYSTUP_kontrola_zamku.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
