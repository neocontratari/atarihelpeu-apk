#!/usr/bin/env python3
"""B345: zapojeni ve zdrojich appky (staticka kontrola).

  B1-B3) BIOS behem stahovani nestartuje: ps1MaybeStartBios konci, dokud bezi stahovani (do logu jednou za
         stahovani), a teprve pak zapina BIOS; priznak stahovani shodi jen POSLEDNI stahovani
  B4)    hra se po stazeni spousti bez BIOSu stejne jako drive hra z cache: bootPs1FileOnCurrentThread ->
         ps1HraDoMonitoru (ps1EnsureBios + bootGameSafe), plochu stavi stranka (ps1SetScreenRect pri nacteni)
  B5-B6) zruseni (odchod ze stranky, RESET, jina hra): stahovani hned konci (drive dojelo na pozadi), uklid jen kdyz
         nebezi novejsi stahovani; po rozbaleni ZIPu zrusena hra zustane v knihovne, ale nespusti se
  M1-M2) misto: jmeno souboru pred kontrolou mista, ZIP 2x + 200 MB, bez ZIPu velikost + 200 MB
  J1-J3) hezke jmeno: katalog N&P / soubor bez pripony; BOOTING ho nese (i vyber souboru), stitek ulozenych pozic
         ps1CurrentGameLabel beze zmeny; mosty ps1RemoteJmeno a ps1RemoteAktivni
  S1-S4) stranka PS1: okno STAHUJI HRU v monitoru (jen kdyz stahovani opravdu bezi), pruh uz jen pro chybu (bez
         surovych stavu a cesty do cache), zavreny panel plochu behem nahravani neukaze, ES5 (bez let/const/=>)
  T1)    preklad: mrtve 'PS1: ...' pryc, novy text panelu LOAD GAME
  N1-N2) napoveda CZ / EN: STAHUJI HRU / DOWNLOADING THE GAME, velikost + 200 MB, ZIP 2x; stejna stavba
  V)     verze 388 "EMU10-B345-PS1-STAHOVANI-BEZ-BIOSU"
Pouziti (z test_overeni/b345): python3 kontrola_b345.py -> VYSTUP_kontrola_b345.txt
"""
import os
import re

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
M = os.path.join(KOREN, 'app/src/main')
J = os.path.join(M, 'java/eu/atarihelp/emu10')
A = os.path.join(M, 'assets')
vystup = []
chyb = 0


def over(ok, co):
    global chyb
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)
    if not ok:
        chyb += 1


def cti(*c):
    with open(os.path.join(*c), encoding='utf-8') as f:
        return f.read()


def metoda(src, hlava):
    if hlava not in src:
        return ''
    a = src.index(hlava)
    i = src.index('{', a)
    hl = 0
    for k in range(i, len(src)):
        if src[k] == '{':
            hl += 1
        elif src[k] == '}':
            hl -= 1
            if hl == 0:
                return src[a:k + 1]
    return src[a:]


def funkce_js(src, jm):
    return metoda(src, 'function ' + jm + '(')


def poradi(src, *casti):
    """vsechny casti v src a v tomhle poradi (chybejici = False, ne vyjimka - at skript dojede i na starem buildu)"""
    k = -1
    for c in casti:
        i = src.find(c, k + 1)
        if i < 0:
            return False
        k = i
    return True


MA = cti(J, 'MainActivity.java')
NP = cti(J, 'NapPravidla.java')

# ---------------- B) BIOS behem stahovani ----------------
bios = metoda(MA, 'private void ps1MaybeStartBios()')
i_guard = bios.find('if (ps1RemoteDownloadActive) {')
i_start = bios.find('ps1BiosStarting = true;')
over(0 < i_guard < i_start and 'return;\n        }\n        ps1BiosStarting = true;' in bios,
     'B1) ps1MaybeStartBios: dokud bezi stahovani, BIOS nestartuje (pojistka PRED ps1BiosStarting = true)')
over('if (ps1BiosCekalGen != g)' in bios and 'B345 PS1 BIOS ceka - stahuje se hra' in bios,
     'B2) "BIOS ceka" do logu jen jednou za stahovani (ps1Status() se pta kazdou vterinu)')
dl = metoda(MA, 'private void startPs1RemoteDownloadAndBoot(final String rawUrl)')
over('mojeStahovani = ++ps1StahovaniGen;' in dl and 'if (mojeStahovani == ps1StahovaniGen) ps1RemoteDownloadActive = false;' in dl
     and dl.count('ps1RemoteDownloadActive = false') == 1,
     'B3) priznak stahovani shodi jen posledni stahovani (konec stareho zruseneho nepusti BIOS do noveho)')
boot = metoda(MA, 'private void bootPs1FileOnCurrentThread(File gameFile, String label, String reason)')
hra = metoda(MA, 'private String ps1HraDoMonitoru(String gamePath)')
stranka = cti(A, 'emu_ps1/index.html')
over('ps1BootActive = true;' in boot and 'ps1HraDoMonitoru(gameFile.getAbsolutePath())' in boot
     and 'ps1EnsureBios(sysDirH);' in hra and 'NativePs1CoreBridge.bootGameSafe(' in hra and 'ps1BiosRunning = false;' in hra
     and 'window.AHPS1.ps1SetScreenRect(l, t, w, h, naSirku);' in stranka and re.search(r'\nensurePs1ScreenAndPoll\(\);', stranka),
     'B4) hra po stazeni: bootPs1FileOnCurrentThread -> ps1HraDoMonitoru (BIOS soubor + bootGameSafe), plochu stavi stranka pri nacteni')
over(poradi(dl, 'if (downloadGen != ps1LifecycleGen || !ps1RemoteDownloadActive)\n                            throw new Ps1StahovaniZruseno(', 'fos.write(buf, 0, n);')
     and 'if (t instanceof Ps1StahovaniZruseno) {' in dl and 'posledni = mojeStahovani == ps1StahovaniGen;' in dl
     and 'static final class Ps1StahovaniZruseno extends IOException' in MA,
     'B5) zruseno -> stahovani hned konci (kontrola pred kazdym zapisem), uklid jen bez novejsiho stahovani')
over(poradi(dl, 'File bootFile = extractPs1RemoteZip(out, dir);', 'ps1WriteCacheMarker(dir, bootFile);',
            'if (downloadGen != ps1LifecycleGen || !ps1RemoteDownloadActive) {', 'B345 PS1 rozbaleno, ale mezitim zruseno',
            'setPs1RemoteStatus("PS1_REMOTE_READY "', 'bootPs1FileOnCurrentThread(bootFile, bootFile.getName(), "remoteZipUrl");'),
     'B6) ZIP rozbalen, ale mezitim zruseno -> hra do knihovny (znacka), nespousti se')

# ---------------- M) misto ----------------
over(poradi(dl, 'String name = safeFileName(guessDownloadName(', 'ps1EnsureFreeSpace(dir, expectedLen, isPs1ZipName(name));')
     and 'ps1EnsureFreeSpace(dir, expectedLen);' not in MA
     and 'NapPravidla.ps1PotrebaMista(contentLength, zip)' in metoda(MA, 'private void ps1EnsureFreeSpace(File dir, long contentLength, boolean zip)'),
     'M1) jmeno souboru se zjisti PRED kontrolou mista, kontrola vi, jestli jde o ZIP')
pm = metoda(NP, 'static long ps1PotrebaMista(long delka, boolean zip)')
over('return (zip ? delka * 2 : delka) + PS1_REZERVA;' in pm and 'return ps1PotrebaMista(delka, true);' in NP
     and 'ps1PotrebaRozbaleni(rozbaleno)' in MA,
     'M2) NapPravidla: bez ZIPu velikost + 200 MB, ZIP 2x + 200 MB (stara funkce = ZIP); rozbaleni ZIPu dal podle skutecnych velikosti')

# ---------------- J) hezke jmeno ----------------
zs = metoda(MA, 'private String ps1JmenoHryZeSouboru(File hra)')
over('NapPravidla.ps1JmenoHry(ps1JmenoZKatalogu(' in zs and 'ps1NejvetsiSoubor(dir)' in zs
     and 'napNazvyHer().get(slozka)' in metoda(MA, 'private String ps1JmenoZKatalogu(String slozka)'),
     'J1) hezke jmeno: katalog N&P podle slozky hry, jinak soubor bez pripony (CUE slozene appkou -> nejvetsi soubor)')
over('setPs1RemoteStatus("PS1_REMOTE_BOOTING #" + cisloStartu + " " + jmeno);' in boot and 'String jmeno = ps1JmenoHryZeSouboru(gameFile);' in boot
     and 'ps1CurrentGameLabel = safeFileName(rawLabel);' in boot
     and '" " + NapPravidla.ps1JmenoHry(null, pickedName, null));' in MA
     and 'ps1JmenoHry = NapPravidla.ps1JmenoHry(katalog, name, null);' in dl,
     'J2) BOOTING nese hezke jmeno (i vyber souboru), stahovani ho nastavi z katalogu; stitek ulozenych pozic beze zmeny')
over(re.search(r'@JavascriptInterface\s+public String ps1RemoteJmeno\(\)', MA) is not None
     and re.search(r'@JavascriptInterface\s+public boolean ps1RemoteAktivni\(\) \{ return ps1RemoteDownloadActive; \}', MA) is not None,
     'J3) mosty AHPS1.ps1RemoteJmeno (hezke jmeno) a AHPS1.ps1RemoteAktivni (bezi stahovani?)')

# ---------------- S) stranka PS1 ----------------
ns = funkce_js(stranka, 'nahravaniStav')
over('var info=stahovaniAktivni()?stahovaniInfo(rs):null;' in ns and "if((!NAHR.on||NAHR.stahuje)&&rs!==NAHR.posledniAuto){" in ns
     and 'ps1RemoteAktivni' in funkce_js(stranka, 'stahovaniAktivni') and 'ps1RemoteJmeno' in funkce_js(stranka, 'ps1JmenoStahovane'),
     'S1) okno STAHUJI HRU v monitoru jen kdyz stahovani opravdu bezi, po stazeni rovnou NAHRAVAM HRU')
ub = funkce_js(stranka, 'updateRemoteBanner')
over('chybaStahovani(st)' in ub and 'compactStatusText' not in ub and 'remotePath' not in stranka and 'remoteTitleFor' not in stranka
     and "'cache: '" not in stranka and '12000' in ub,
     'S2) pruh pod monitorem jen pro chybu (bez surovych stavu a cesty do cache), stejna chyba jednou na 12 s')
over('if(!NAHR.on){' in funkce_js(stranka, 'napPlochaShow') and 'NAHR.stahuje=false;' in funkce_js(stranka, 'nahravaniKonec'),
     'S3) zavreny panel behem nahravani / stahovani plochu jadra neukaze')
skripty = '\n'.join(re.findall(r'<script>([\s\S]*?)</script>', stranka))
nove = '\n'.join(funkce_js(stranka, f) for f in ('nahravaniStav', 'stahovaniInfo', 'nahravaniStahuji', 'chybaStahovani',
                                                    'updateRemoteBanner', 'TX', 'mbText', 'stahovaniAktivni', 'napPlochaShow'))
over(nove and not re.search(r'\b(let|const)\s|=>|`', nove) and len(skripty) > 1000,
     'S4) nove funkce stranky v ES5 (bez let / const / => / `) - WebView na TV')

# ---------------- T) preklad ----------------
en = cti(A, 'jazyk/en_ps1.js')
over("'PS1: stahuji hru'" not in en and "'Cekam...'" not in en
     and 'and starts by itself, next time it starts straight from the phone.' in en
     and 'Poprvé se hra stáhne (volno asi velikost hry + 200 MB, ZIP 2× velikost + 200 MB) a sama se spustí' in stranka,
     'T1) en_ps1.js: mrtve texty stareho pruhu pryc, novy text panelu LOAD GAME CZ / EN')

# ---------------- N) napoveda ----------------
cz, enh = cti(A, 'napoveda/index.html'), cti(A, 'napoveda/en.html')
over('<span class="t">STAHUJI HRU</span>' in cz and 'Konzole (BIOS) se mezitím nezapíná – hra se po stažení spustí sama.' in cz
     and 'asi velikost hry + 200 MB, hra v ZIPu chce asi 2× velikost hry + 200 MB' in cz
     and 'Stažení PS1 hry chce volno asi její velikost + 200 MB (hra v ZIPu asi 2× velikost + 200 MB)' in cz
     and '<span class="t">DOWNLOADING THE GAME</span>' in enh and 'The console (BIOS) does not start meanwhile – the game starts by itself once downloaded.' in enh
     and "about the game's size + 200 MB, a game in a ZIP about 2× the game's size + 200 MB" in enh
     and 'Downloading a PS1 game needs about its size + 200 MB free (a game in a ZIP about 2× its size + 200 MB)' in enh,
     'N1) napoveda CZ / EN: obrazovka pri stahovani, BIOS ceka, misto velikost + 200 MB / ZIP 2x (PS1 i TV)')


def stavba(t):
    t = re.sub(r'<!--[\s\S]*?-->', '', t)
    return re.findall(r'<(h[1-4]|li|dt|dd|p|section|ul|ol|dl)\b', t)


over(stavba(cz) == stavba(enh), 'N2) napoveda CZ a EN maji stejnou stavbu (%d prvku)' % len(stavba(cz)))

# ---------------- V) verze ----------------
g = cti(KOREN, 'app/build.gradle')
over('versionCode 388' in g and 'versionName "EMU10-B345-PS1-STAHOVANI-BEZ-BIOSU"' in g, 'V) verze 388 "EMU10-B345-PS1-STAHOVANI-BEZ-BIOSU"')

vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb))
print(vystup[-1])
with open(os.path.join(ZDE, 'VYSTUP_kontrola_b345.txt'), 'w', encoding='utf-8') as f:
    f.write('\n'.join(vystup) + '\n')
raise SystemExit(1 if chyb else 0)
