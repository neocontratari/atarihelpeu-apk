# B326 - kontrola zapojeni volby jazyka v kodu appky (bez telefonu):
#   K1 onCreate konci volbou jazyka (nic jineho se pred ni nenacita), K2 napVolbaJazyka, K3 zamek aktivace
#   strankou jazyka nebrani, K4 most AHJAZYK.prvniVolba, K5 puvodni start (B325) presunuty BEZE ZMENY do
#   napStartPoJazyku, K6 stranka a jeji most, K7 verze, K8 skript stranky je ES5 (acorn, stare WebView).
# Pouziti: python3 kontrola_b326.py [PRAC]   -> VYSTUP_kontrola_b326.txt   (PRAC = kam doinstalovat acorn)
import os, re, subprocess, sys

ZDE = os.path.dirname(os.path.abspath(__file__))
R = os.path.abspath(os.path.join(ZDE, '..', '..'))
PRAC = sys.argv[1] if len(sys.argv) > 1 else '/tmp/nap_b326'
MA = 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java'
vystup, chyb = [], 0


def kontrola(nazev, ok, detail=''):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + nazev + ('' if ok or detail == '' else '  -> ' + str(detail)[:400]))


def metoda(src, hlavicka):
    """Telo metody od hlavicky po odpovidajici zaviraci zavorku."""
    i = src.index(hlavicka)
    j = src.index('{', i)
    hloubka = 0
    for k in range(j, len(src)):
        if src[k] == '{':
            hloubka += 1
        elif src[k] == '}':
            hloubka -= 1
            if hloubka == 0:
                return src[i:k + 1]
    raise ValueError(hlavicka)


src = open(os.path.join(R, MA), encoding='utf-8').read()
vystup.append('B326 - zapojeni volby jazyka pri prvnim spusteni (MainActivity, NapJazyk, hry/jazyk.html)')
vystup.append('')

oc = metoda(src, 'protected void onCreate(Bundle savedInstanceState)')
konec = oc[oc.index('setContentView(rootFrame);'):]
prikazy = [r.strip() for r in konec.split('\n') if r.strip() and not r.strip().startswith('//')]
kontrola('K1 onCreate konci: setContentView -> if (!NapJazyk.zvoleno(this)) napVolbaJazyka(); else napStartPoJazyku();',
         prikazy == ['setContentView(rootFrame);', 'if (!NapJazyk.zvoleno(this)) napVolbaJazyka();', 'else napStartPoJazyku();', '}'], prikazy)
kontrola('K1 v onCreate se zadna stranka nenacita primo (jen v obsluze WebView), takze volba jazyka je prvni obrazovka',
         not re.search(r'^\s{8}web\.loadUrl\(', oc, re.M), '')

vj = metoda(src, 'private void napVolbaJazyka()')
kontrola('K2 napVolbaJazyka: napCekaNaJazyk = true, nacte NAP_JAZYK_URL = hry/jazyk.html',
         'napCekaNaJazyk = true;' in vj and 'web.loadUrl(NAP_JAZYK_URL);' in vj
         and 'private static final String NAP_JAZYK_URL = "file:///android_asset/hry/jazyk.html";' in src, vj)

zs = metoda(src, 'private boolean napZamekStranky(String url)')
kontrola('K3 nesparovany telefon smi na stranku jazyka (napZamekStranky ji nepresmeruje na aktivaci)',
         '!url.startsWith(NAP_JAZYK_URL)' in zs, zs)

pv = metoda(src, '@JavascriptInterface public void prvniVolba(String novy)')
kontrola('K4 most AHJAZYK.prvniVolba: jen "cs" = cesky, cokoli jineho anglicky; ulozi (NapJazyk.nastav), jazyk do C++',
         'NapJazyk.CS.equals(' in pv and '? NapJazyk.CS : NapJazyk.EN' in pv and 'NapJazyk.nastav(MainActivity.this, k)' in pv
         and 'jazykDoKnihoven(' in pv, pv)
kontrola('K4 start pokracuje jen jednou (napCekaNaJazyk -> false -> napStartPoJazyku); stranka znovu (ZPET) = aktivace / menu',
         re.search(r'if \(napCekaNaJazyk\) \{\s*napCekaNaJazyk = false;[^}]*napStartPoJazyku\(\);', pv) is not None
         and 'web.loadUrl(napZamceno() ? NAP_AKTIVACE_URL : "file:///android_asset/index.html")' in pv, pv)
kontrola('K4 most AHJAZYK je v appce zaregistrovany a OPTIONS (nastav) zustava',
         'web.addJavascriptInterface(new AHJazyk(), "AHJAZYK");' in src and '@JavascriptInterface public String nastav(String novy)' in src)

stary = subprocess.run(['git', '-C', R, 'show', 'HEAD:' + MA], capture_output=True, text=True).stdout
if 'napStartPoJazyku' in stary:   # uz zapsane B326: porovnat s B325
    stary = subprocess.run(['git', '-C', R, 'show', '46fcfc8:' + MA], capture_output=True, text=True).stdout
ZAC, KON = '        // BUILD2SA21: uvodni intro pro sraz klanu KKT.', '    private void napStartRozhodniACini('
blok_b325 = stary[stary.index(ZAC):stary.index(KON)]
blok_b326 = src[src.index(ZAC):src.index(KON)]
kontrola('K5 puvodni start B325 (otazka na stazeni, aktivace, intro, menu) je v napStartPoJazyku znak po znaku stejny (%d znaku)' % len(blok_b325),
         blok_b325 == blok_b326 and src.index('private void napStartPoJazyku()') < src.index(ZAC) < src.index(KON), (len(blok_b325), len(blok_b326)))

st = open(os.path.join(R, 'app/src/main/assets/hry/jazyk.html'), encoding='utf-8').read()
kontrola('K6 stranka hry/jazyk.html: ENGLISH (autofocus) pred CESTINA, vola AHJAZYK.prvniVolba',
         st.index('>ENGLISH</button>') < st.index('>ČEŠTINA</button>') and 'id="en" class="tlacitko" type="button" autofocus' in st
         and 'window.AHJAZYK.prvniVolba(k)' in st)
nj = open(os.path.join(R, 'app/src/main/java/eu/atarihelp/emu10/NapJazyk.java'), encoding='utf-8').read()
kontrola('K6 NapJazyk: nezvoleno = EN (ne podle telefonu), zvoleno() = ulozena volba',
         'if (k == null) k = EN;' in nj and 'k = podleTelefonu()' not in nj and 'return c != null && ulozeny(c) != null;' in nj)

bg = open(os.path.join(R, 'app/build.gradle'), encoding='utf-8').read()
kontrola('K7 build.gradle: versionCode 369, versionName EMU10-B326-JAZYK', 'versionCode 369' in bg and 'versionName "EMU10-B326-JAZYK"' in bg)

os.makedirs(PRAC, exist_ok=True)
if not os.path.isdir(os.path.join(PRAC, 'node_modules', 'acorn')):
    subprocess.run(['npm', 'install', '--silent', 'acorn@8.11.3'], cwd=PRAC, capture_output=True)
skript = re.findall(r'<script>(.*?)</script>', st, re.S)
open(os.path.join(PRAC, 'jazyk_skript.js'), 'w', encoding='utf-8').write('\n'.join(skript))
r = subprocess.run(['node', '-e', "const a=require('acorn'),fs=require('fs');try{a.parse(fs.readFileSync(process.argv[1],'utf8'),{ecmaVersion:5});console.log('ES5 OK')}catch(e){console.log('NE ES5: '+e.message)}",
                    os.path.join(PRAC, 'jazyk_skript.js')], cwd=PRAC, capture_output=True, text=True)
kontrola('K8 skript stranky je ES5 (acorn ecmaVersion 5 - stare WebView)', len(skript) == 1 and r.stdout.strip() == 'ES5 OK', (r.stdout + r.stderr).strip())

vystup.append('')
vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (sum(1 for x in vystup if x.startswith(('OK', 'CHYBA'))), chyb))
open(os.path.join(ZDE, 'VYSTUP_kontrola_b326.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
print('\n'.join(vystup))
sys.exit(1 if chyb else 0)
