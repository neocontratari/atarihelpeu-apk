#!/usr/bin/env python3
"""B339: kontrola zapojeni (OPTIONS -> ULOZISTE, TV: PS1 HRY V TELEVIZI, PS1 stahovani pri malo mistu).
Pouziti: python3 test_overeni/b339/kontrola_b339.py   -> VYSTUP_kontrola_b339.txt
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


# ---- U) uloziste ----
ul = blok(MA, '        public String uloziste() {', '\n        }\n')
over('u == null || !u.startsWith("file:///android_asset/")' in ul and 'getUsableSpace()' in ul and 'getTotalSpace()' in ul
     and 'ps1KnihovnaHry()' in ul and 'NapPravidla.ulozistePopis(volno, celkem, hry.size(), ps1, napJeTv(), NapJazyk.en(MainActivity.this))' in ul,
     'U1 AHRENDER.uloziste: jen stranky appky, volno / celkem, PS1 hry, veta z NapPravidla (CZ / EN, TV / zarizeni)')
over('<div class="stlabel2">ÚLOŽIŠTĚ</div>' in IX and '<div class="sthint" id="stUloziste" data-nepreklad></div>' in IX
     and "try{ ulozisteObnov(); }catch(e){}" in blok(IX, 'function openSettings(){', '\n// B320')
     and 'NapJazyk.priZmene(ulozisteObnov)' in IX and "'ÚLOŽIŠTĚ': 'STORAGE'" in EN,
     'U2 OPTIONS: ULOZISTE (EN STORAGE), obnova pri kazdem otevreni OPTIONS a pri zmene jazyka')
du = blok(MA, '    private void napDiagnostikaUloziste() {', '\n    }\n')
over('"B339 ULOZISTE: volno "' in du and 'new Thread(() -> {' in du and '"nap-uloziste"' in du,
     'U3 log pri startu "B339 ULOZISTE: volno ... z ... | PS1 her" (mimo hlavni vlakno)')

# ---- T) TV: HRY V TELEVIZI ----
km = blok(MA, '    private void konzoleTvMenu(final boolean ps1) {', '\n    }\n')
i_np = km.index('tx("HRY ZE SERVERU N&P", "GAMES FROM THE N&P SERVER"),')
i_tv = km.index('tx("HRY V TELEVIZI (stažené – spustit / smazat)", "GAMES ON THIS TV (downloaded – play / delete)"),')
i_ul = km.index('tx("ULOŽIT POZICI (SAVE GAME)"')
over(i_np < i_tv < i_ul and 'final int[] akce = ps1 ? new int[]{0, 4, 1, 2, 3, -1} : new int[]{0, 1, 2, -1};' in km
     and 'akce[w]' in km and km.count('tx("HRY V TELEVIZI') == 1,
     'T1 menu konzole PS1: HRY V TELEVIZI hned po HRY ZE SERVERU N&P (akce 4), Sega beze zmeny')
ka = blok(MA, '    private void konzoleTvAkce(boolean ps1, int akce) {', '\n    }\n')
over('case 4:' in ka and 'if (ps1) ps1HryVTv();' in ka, 'T2 akce 4 -> ps1HryVTv (jen PS1)')
hv = blok(MA, '    private void ps1HryVTv() {', '\n    }\n')
over('new Thread(() -> {' in hv and 'ps1KnihovnaHry()' in hv and 'ui.post(() -> ps1HryVTvUkaz(hry, volno, celkem));' in hv,
     'T3 HRY V TELEVIZI: seznam a misto mimo hlavni vlakno, dialog na hlavnim')
hu = blok(MA, '    private void ps1HryVTvUkaz(', '\n    }\n')
over('hry.isEmpty()' in hu and 'napOtevriHry("ps1")' in hu and 'ps1HraVTv(hry.get(w))' in hu and 'uložená pozice' in hu,
     'T4 seznam: nazev, velikost, ulozena pozice; prazdny -> HRY ZE SERVERU N&P')
ha = blok(MA, '    private void ps1HraVTv(final Ps1Hra h) {', '\n    }\n')
over('tx("▶ SPUSTIT", "▶ PLAY")' in ha and 'ps1HraVTvSpust(h)' in ha and 'ps1HraVTvSmaz(h, w == 2)' in ha
     and 'SMAZAT HRU I ULOŽENOU POZICI' in ha and '(d, w) -> ps1HryVTv()' in ha,
     'T5 hra: SPUSTIT / SMAZAT HRU / SMAZAT HRU I ULOZENOU POZICI, ZPET = seznam')
hs = blok(MA, '    private void ps1HraVTvSpust(final Ps1Hra h) {', '\n    }\n')
over('isPs1OwnerUrl(u)' in hs and 'ps1LibraryLaunchByKey(h.key)' in hs and 'new Thread(' in hs,
     'T6 SPUSTIT = ps1LibraryLaunchByKey (jako ▶ v CD ISO), jen na strance PS1, nahravani si stranka zapne sama (PS1_REMOTE_BOOTING)')
hz = blok(MA, '    private void ps1HraVTvSmaz(final Ps1Hra h, final boolean sPozici) {', '\n    }\n')
over('ps1LibraryDeleteByKey(h.key, sPozici)' in hz and 'new Thread(() -> {' in hz and 'ps1HryVTv();' in hz,
     'T7 SMAZAT = ps1LibraryDeleteByKey (jako kos v CD ISO) mimo hlavni vlakno, pak seznam znovu (nove volno)')

# ---- S) stahovani PS1 pri malo mistu ----
ef = blok(MA, '    private void ps1EnsureFreeSpace(File dir, long contentLength) throws IOException {', '\n    }\n')
om = blok(MA, '    private void ps1OverMisto(File dir, long potreba, String co) throws IOException {', '\n    }\n')
over('NapPravidla.ps1PotrebaMista(contentLength)' in ef and 'NapPravidla.dostMista(volno, potreba)' in om
     and 'throw new Ps1MaloMista(potreba, volno, NapPravidla.ps1MaloMista(potreba, volno, napJeTv(), NapJazyk.en(this)));' in om
     and 'PS1 cache byla prave automaticky uklizena -' not in MA and '"MALO MISTA v telefonu:' not in MA,
     'S1 MALO MISTA: hlaska z NapPravidla (jazyk, TV / telefon), stara veta "v telefonu ... uklizena" pryc')
zx = blok(MA, '    private File extractPs1RemoteZip(File zipFile, File dir) throws IOException {', '\n    }\n')
i_bios = zx.index('throw new IOException("BIOS NAINSTALOVAN')
i_misto = zx.index('ps1OverMisto(dir, NapPravidla.ps1PotrebaRozbaleni(rozbaleno)')
i_cue = zx.index('java.util.zip.ZipEntry cue = null;')
over(i_bios < i_misto < i_cue and 'isPs1ZipPayloadName(zs.getName()) && zs.getSize() > 0' in zx,
     'S2 ZIP: misto na rozbaleni podle velikosti v ZIPu - po rozpoznani BIOSu, PRED rozbalovanim')
st = blok(MA, '    private void startPs1RemoteDownloadAndBoot(final String rawUrl) {', '\n    }\n')
cat = blok(st, '            } catch (Throwable t) {', '            } finally {')
i_bios2 = cat.index('startsWith("BIOS NAINSTALOVAN")')
i_ukl = cat.index('ps1UklidNedokonceneho(hraDir')
over('hraDir = dir;' in st and i_bios2 < i_ukl and 'moje = downloadGen == ps1LifecycleGen;' in cat and 'if (moje) ps1UklidNedokonceneho' in cat
     and 'if (t instanceof Ps1MaloMista && napJeTv())' in cat and 'ps1MaloMistaTv(mm.potreba, mm.volno)' in cat,
     'S3 chyba stahovani: uklid nedokoncene hry (jen kdyz nezacalo jine stahovani; BIOS ZIP mimo), na TV dialog MALO MISTA')
uk = blok(MA, '    private void ps1UklidNedokonceneho(File dir, String proc) {', '\n    }\n')
over('ps1RemoteCacheMarker(dir).isFile()) return;' in uk and 'deleteTree(dir)' in uk,
     'S4 uklid maze jen slozku BEZ znacky _ps1_boot.name (hotova hra se nesmaze nikdy)')
mm = blok(MA, '    private void ps1MaloMistaTv(long potreba, long volno) {', '\n    }\n')
over('tx("HRY V TELEVIZI", "GAMES ON THIS TV"), (d, w) -> ps1HryVTv()' in mm, 'S5 TV dialog MALO MISTA -> HRY V TELEVIZI')

# ---- L) knihovna ----
kh = blok(MA, '    private java.util.List<Ps1Hra> ps1KnihovnaHry() {', '\n    }\n')
lj = blok(MA, '    private String ps1LibraryListJson() {', '\n    }\n')
over('dirSizeBytes(dir)' in kh and 'if (boot == null) continue;' in kh and 'NapPravidla.ps1JmenoHry(' in kh and 'napNazvyHer()' in kh
     and 'ps1KnihovnaHry()' in lj and 'NapJson.uvoz(h.nazev)' in lj and 'ma_ulozenou_hru' in lj,
     'L1 knihovna (CD ISO i TV): velikost cele slozky, jmeno z katalogu N&P / souboru, nedokoncene se neukazuji; JSON pro stranku stejny')
for nazev, f in (('NapPravidla', PR),):
    over('static String ps1MaloMista(long potreba, long volno, boolean tv, boolean en)' in f
         and 'static String ulozistePopis(' in f and 'static long ps1PotrebaRozbaleni(long rozbaleno)' in f,
         'L2 %s: ciste funkce (JVM test TestB339)' % nazev)

# ---- N) napoveda, verze ----
over('<b>PS1 na TV a místo v televizi:</b>' in NCZ and '<span class="t">HRY V TELEVIZI</span>' in NCZ and '<span class="t">ÚLOŽIŠTĚ</span>' in NCZ
     and '<b>PS1 on the TV and space on the TV:</b>' in NEN and '<span class="t">GAMES ON THIS TV</span>' in NEN and '<span class="t">STORAGE</span>' in NEN,
     'N1 napoveda CZ / EN: HRY V TELEVIZI, ULOZISTE')
over('versionCode 382' in GR and 'versionName "EMU10-B339-TV-ULOZISTE-PS1"' in GR, 'N2 build.gradle: versionCode 382 "EMU10-B339-TV-ULOZISTE-PS1"')

js = '\n'.join(re.findall(r'<script>([\s\S]*?)</script>', IX))
r = subprocess.run(['node', '-e', "const a=require('/opt/npm-tools/node_modules/acorn');a.parse(process.argv[1],{ecmaVersion:5});console.log('ok')", js],
                   capture_output=True, text=True, timeout=60)
over(r.stdout.strip() == 'ok', 'E skripty index.html jsou ES5%s' % ('' if r.stdout.strip() == 'ok' else ': ' + r.stderr[-200:]))

vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (sum(1 for v in vystup if v.startswith(('OK', 'CHYBA'))), chyb))
print('\n'.join(vystup))
open(os.path.join(ZDE, 'VYSTUP_kontrola_b339.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
