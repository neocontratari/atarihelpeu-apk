# B327 - kontrola zapojeni v kodu appky (bez telefonu):
#   volba jazyka jednou KAZDEMU (obrazovkaHotova / oznacObrazovku), okno aktualizace az do instalace
#   (ZRUSIT / ZAVRIT, vysledek instalace), otevreni po aktualizaci (Android 9-), "Nainstalovana nova
#   verze" pri startu, zadny slib "appka se sama otevre" pro Android 10+, puvodni start B325 beze zmeny.
# Pouziti: python3 kontrola_b327.py   -> VYSTUP_kontrola_b327.txt
import os, re, subprocess, sys

ZDE = os.path.dirname(os.path.abspath(__file__))
R = os.path.abspath(os.path.join(ZDE, '..', '..'))
J = os.path.join(R, 'app/src/main/java/eu/atarihelp/emu10')
vystup, chyb = [], 0


def kontrola(nazev, ok, detail=''):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + nazev + ('' if ok or detail == '' else '  -> ' + str(detail)[:400]))


def metoda(src, hlavicka):
    i = src.index(hlavicka)
    j = src.index('{', i)
    h = 0
    for k in range(j, len(src)):
        if src[k] == '{':
            h += 1
        elif src[k] == '}':
            h -= 1
            if h == 0:
                return src[i:k + 1]
    raise ValueError(hlavicka)


cti = lambda p: open(os.path.join(R, p), encoding='utf-8').read()
ma = cti('app/src/main/java/eu/atarihelp/emu10/MainActivity.java')
na = cti('app/src/main/java/eu/atarihelp/emu10/NapAktualizace.java')
npa = cti('app/src/main/java/eu/atarihelp/emu10/NapPoAktualizaci.java')
nj = cti('app/src/main/java/eu/atarihelp/emu10/NapJazyk.java')
mf = cti('app/src/main/AndroidManifest.xml')
vystup.append('B327 - zapojeni: volba jazyka jednou kazdemu, okno aktualizace, otevreni po aktualizaci')
vystup.append('')

oc = metoda(ma, 'protected void onCreate(Bundle savedInstanceState)')
konec = [r.strip() for r in oc[oc.index('setContentView(rootFrame);'):].split('\n') if r.strip() and not r.strip().startswith('//')]
kontrola('K1 onCreate konci: if (!NapJazyk.obrazovkaHotova(this)) napVolbaJazyka(); else napStartPoJazyku();',
         konec == ['setContentView(rootFrame);', 'if (!NapJazyk.obrazovkaHotova(this)) napVolbaJazyka();', 'else napStartPoJazyku();', '}'], konec)
pv = metoda(ma, '@JavascriptInterface public void prvniVolba(String novy)')
kontrola('K2 prvniVolba: ulozi jazyk a oznaci obrazovku (priste uz ne), pak start appky',
         pv.index('NapJazyk.nastav(MainActivity.this, k)') < pv.index('NapJazyk.oznacObrazovku(MainActivity.this)') < pv.index('napStartPoJazyku();'), pv)
kontrola('K2 NapJazyk: obrazovkaHotova = zaznam "obrazovka_volby" == "1" (B326 ho nemela -> jednou kazdy)',
         'K_OBRAZOVKA = "obrazovka_volby"' in nj and '"1".equals(p(c).getString(K_OBRAZOVKA, null))' in nj
         and 'p(c).edit().putString(K_OBRAZOVKA, "1").apply()' in nj)

stary = subprocess.run(['git', '-C', R, 'show', '46fcfc8:app/src/main/java/eu/atarihelp/emu10/MainActivity.java'], capture_output=True, text=True).stdout
ZAC, KON = '        // BUILD2SA21: uvodni intro pro sraz klanu KKT.', '    private void napStartRozhodniACini('
kontrola('K3 puvodni start B325 (stazeni, aktivace, intro, menu) v napStartPoJazyku porad znak po znaku stejny',
         stary[stary.index(ZAC):stary.index(KON)] == ma[ma.index(ZAC):ma.index(KON)])

kontrola('K4 manifest: prijimac NapPoAktualizaci na MY_PACKAGE_REPLACED',
         re.search(r'<receiver\s+android:name="eu\.atarihelp\.emu10\.NapPoAktualizaci"\s+android:exported="true">\s*<intent-filter>\s*'
                   r'<action android:name="android\.intent\.action\.MY_PACKAGE_REPLACED" />', mf) is not None)
kontrola('K5 NapPoAktualizaci: jen MY_PACKAGE_REPLACED, jen do Androidu 9 (otevreSeSama), otevre MainActivity s priznakem',
         'Intent.ACTION_MY_PACKAGE_REPLACED.equals(i.getAction())' in npa and 'NapAktualizaceTexty.otevreSeSama(Build.VERSION.SDK_INT)' in npa
         and 'new Intent(c, MainActivity.class)' in npa and 'FLAG_ACTIVITY_NEW_TASK' in npa and 'o.putExtra(EXTRA, true)' in npa)

slib = []
for koren, _, soubory in os.walk(os.path.join(R, 'app/src/main')):
    if '/cpp' in koren:
        continue
    for f in soubory:
        if f.endswith(('.java', '.html', '.js', '.txt')):
            t = open(os.path.join(koren, f), encoding='utf-8', errors='replace').read()
            for vzor in ('sama zavře a otevře', 'zavře a otevře znovu', 'close and open again by itself', 'closes and opens again in the new version'):
                if vzor in t:
                    slib.append(os.path.relpath(os.path.join(koren, f), R) + ': ' + vzor)
kontrola('K6 nikde (Java, napoveda) uz neni nepravdivy slib "appka se sama zavre a otevre znovu"', slib == [], slib)

ni = metoda(ma, 'private void napInstalace(Intent i)')
kontrola('K7 napInstalace: okno aktualizace podle vysledku (NapAktualizace.vysledekInstalace), zruseni = srozumitelna hlaska',
         'NapAktualizace.vysledekInstalace(this, st)' in ni and 'STATUS_FAILURE_ABORTED' in ni and 'Instalace zrušena' in ni, ni)
kontrola('K8 onCreate: NapAktualizace.poStartu (Toast "Nainstalovana nova verze") + zapis "otevrena sama" do logu',
         'NapAktualizace.poStartu(this)' in oc and 'getBooleanExtra(NapPoAktualizaci.EXTRA, false)' in oc)

ns = metoda(na, 'private static void nainstaluj(final Activity a, final String zipUrl, final Hlaska log)')
ok_blok = ns[ns.index('if (v.startsWith("OK")) {'):]
ok_blok = ok_blok[:ok_blok.index('return;') + 7]
kontrola('K9 po predani Androidu okno ZUSTAVA (text poPredani, tlacitko ZAVRIT) - zavira se jen pri chybe / zruseni',
         'dlg.dismiss()' not in ok_blok and 'NapAktualizaceTexty.poPredani(en, Build.VERSION.SDK_INT, hotovo)' in ok_blok
         and '"ZAVŘÍT", "CLOSE"' in ok_blok, ok_blok)
kontrola('K10 ZRUSIT behem stahovani: priznak + preruseni spojeni (necekat na data), relace se zahodi, hlaska "Aktualizace zrusena"',
         'zrusit = true;' in ns and 's0.disconnect()' in ns and 'relace.abandon()' in ns and 'Aktualizace zrušena' in ns
         and 'vysledek = zrusit ? "ZRUSENO' in ns)
kontrola('K11 behem stahovani text s MB / procenty / rychlosti (NapAktualizaceTexty.stahovani), obnova nejvys 4x za sekundu',
         'NapAktualizaceTexty.stahovani(en, primoApk, staženo, celkem, zaS)' in ns and 'ted - posledniUi >= 250L' in ns)
bg = cti('app/build.gradle')
kontrola('K12 build.gradle: versionCode 370, versionName EMU10-B327-INSTALACE', 'versionCode 370' in bg and 'versionName "EMU10-B327-INSTALACE"' in bg)

vystup.append('')
vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (sum(1 for x in vystup if x.startswith(('OK', 'CHYBA'))), chyb))
open(os.path.join(ZDE, 'VYSTUP_kontrola_b327.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
print('\n'.join(vystup))
sys.exit(1 if chyb else 0)
