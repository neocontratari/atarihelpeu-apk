# B331: zapojeni Bluetooth ovladace v kodu appky (staticka kontrola zdroju) - co testy na PC neprojdou.
# Spusteni z test_overeni/b331: python3 kontrola_b331.py  -> VYSTUP_kontrola_b331.txt
import io, os, re, sys
import xml.dom.minidom as md

KOREN = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
ok = chyb = 0


def cti(p):
    return io.open(os.path.join(KOREN, p), encoding='utf-8').read()


def kontrola(co, v):
    global ok, chyb
    print(('OK     ' if v else 'CHYBA  ') + co)
    if v:
        ok += 1
    else:
        chyb += 1


ma = cti('app/src/main/java/eu/atarihelp/emu10/MainActivity.java')
bt = cti('app/src/main/java/eu/atarihelp/emu10/NapBluetooth.java')
mf = cti('app/src/main/AndroidManifest.xml')
ix = cti('app/src/main/assets/index.html')
en = cti('app/src/main/assets/jazyk/en_menu.js')
hc = cti('app/src/main/assets/napoveda/index.html')
he = cti('app/src/main/assets/napoveda/en.html')
bg = cti('app/build.gradle')

# ---- manifest ----
d = md.parseString(mf.encode('utf-8'))
prava = {}
for e in d.getElementsByTagName('uses-permission'):
    prava[e.getAttribute('android:name')] = (e.getAttribute('android:maxSdkVersion'), e.getAttribute('android:usesPermissionFlags'))
kontrola('M1 manifest: BLUETOOTH a BLUETOOTH_ADMIN jen do Androidu 11 (maxSdkVersion 30)',
         prava.get('android.permission.BLUETOOTH', ('', ''))[0] == '30' and prava.get('android.permission.BLUETOOTH_ADMIN', ('', ''))[0] == '30')
kontrola('M2 manifest: Android 12+ BLUETOOTH_SCAN s neverForLocation (hledani BEZ polohy) a BLUETOOTH_CONNECT',
         prava.get('android.permission.BLUETOOTH_SCAN', ('', ''))[1] == 'neverForLocation' and 'android.permission.BLUETOOTH_CONNECT' in prava)
kontrola('M3 manifest: poloha jen do Androidu 11 (kvuli hledani Bluetooth), Bluetooth neni podminkou instalace',
         prava.get('android.permission.ACCESS_FINE_LOCATION', ('', ''))[0] == '30' and prava.get('android.permission.ACCESS_COARSE_LOCATION', ('', ''))[0] == '30'
         and any(e.getAttribute('android:name') == 'android.hardware.bluetooth' and e.getAttribute('android:required') == 'false'
                 for e in d.getElementsByTagName('uses-feature')))

# ---- MainActivity ----
kontrola('A1 MainActivity: NapBluetooth vznikne po ovladaci (zna pripojene ovladace), do logu souhrn pri startu',
         'bt = new NapBluetooth(this, ovladac, this::appendNativeLog);' in ma and 'appendNativeLog("B331 BT: " + bt.souhrn());' in ma
         and ma.index('ovladac.start();') < ma.index('bt = new NapBluetooth('))
kontrola('A2 JS rozhrani AHBT: hledej, sparuj, zastav, stav, nastaveni, poloha',
         'web.addJavascriptInterface(new AHBluetooth(), "AHBT");' in ma
         and all(('@JavascriptInterface public ' + m) in ma for m in ['void hledej()', 'void sparuj(String adresa)', 'void zastav()', 'String stav()', 'void nastaveni()', 'void poloha()']))
kontrola('A3 vysledek povoleni a okna "zapnout Bluetooth" jde do NapBluetooth (a nic jineho ho nezpracuje)',
         'if (requestCode == NapBluetooth.REQ_POVOLENI) {' in ma and 'bt.povoleniVysledek(grantResults);' in ma
         and 'if (req == NapBluetooth.REQ_ZAPNUTI) {' in ma and 'bt.zapnutiVysledek(res);' in ma
         and not re.search(r'static final int PICK_[A-Z_0-9]+ *= *3[12];', ma))
od = ma[ma.index('protected void onDestroy()'):ma.index('protected void onResume()')]
kontrola('A4 konec appky: hledani zastavit a prijimac odhlasit (onDestroy, hned ve vlakne UI)',
         'bt.zastav();' in od and 'if (Looper.myLooper() == Looper.getMainLooper()) zastavUi();' in bt)

# ---- NapBluetooth ----
hl = bt[bt.index('private void hledejUi()'):bt.index('void povoleniVysledek(')]
poradi = [hl.index('chybejiciPovoleni()'), hl.index('ad.isEnabled()'), hl.index('potrebujePolohuZapnutou'), hl.index('ad.startDiscovery()')]
kontrola('B1 postup hledani: povoleni -> zapnuti Bluetooth (systemove okno) -> Poloha (Android 10/11) -> hledani',
         poradi == sorted(poradi) and 'a.requestPermissions(chybi, REQ_POVOLENI);' in hl
         and 'new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE), REQ_ZAPNUTI' in hl)
pr = bt[bt.index('private void pridej('):bt.index('private void hledaniSkoncilo()')]
kontrola('B2 do logu jen nalezene OVLADACE a konec adresy (jmena cizich zarizeni v okoli ne)',
         'if (novyOvladac)' in pr and pr.count('log(') == 1 and 'kratkaAdresa(adr)' in pr and 'MAX_NALEZENYCH' in pr)
kontrola('B3 parovani: createBond, uz sparovany = jen zapnout; stav parovani z Androidu (sparovano / nepovedlo)',
         'final boolean ok = d.createBond();' in bt and 'BluetoothDevice.BOND_BONDED' in bt
         and 'stav == BluetoothDevice.BOND_NONE && predtim == BluetoothDevice.BOND_BONDING' in bt)
st = bt[bt.index('String stav() {'):bt.index('String souhrn() {')]
kontrola('B4 "pripojeny": sparovany ovladac se objevi jako vstupni zarizeni (NapOvladac.jmenaPripojenych) -> PRIPOJENO',
         'ovladac.jmenaPripojenych()' in st and 'faze = NapBluetoothTexty.F_PRIPOJENO;' in st)
kontrola('B5 zapamatovane ovladace: sparovane v Androidu (getBondedDevices), Android 12+ jen s povolenim BLUETOOTH_CONNECT',
         'ad.getBondedDevices()' in st and 'Build.VERSION.SDK_INT < 31' in st and 'BLUETOOTH_CONNECT' in st)
kontrola('B6 zadne volani, ktere Android 7 (minSdk 24) nema (String.join, isLocationEnabled bez kontroly verze)',
         'String.join' not in bt and 'Build.VERSION.SDK_INT < 28 || lm.isLocationEnabled()' in bt)

# ---- stranka, jazyk, napoveda, verze ----
kontrola('S1 OPTIONS: tlacitko PRIPOJIT OVLADAC (BLUETOOTH), seznam a stav bez prekladace (data-nepreklad), zavreni OPTIONS zastavi hledani',
         'id="stBtHledej"' in ix and 'id="stBtStav" data-nepreklad' in ix and 'id="stBtSeznam" data-nepreklad' in ix
         and 'try{ btZavri(); }catch(e){}' in ix and 'try{ btObnov(); }catch(e){}' in ix)
kontrola('S2 jmena zarizeni jen jako text (textContent, zadne innerHTML v Bluetooth casti)',
         'innerHTML' not in ix[ix.index('// B331: BLUETOOTH OVLADAC PRIMO V APPCE'):ix.index('// B331: BLUETOOTH OVLADAC PRIMO V APPCE') + 4000])
kontrola('S3 anglicky slovnik: CONNECT CONTROLLER (BLUETOOTH), BLUETOOTH SETTINGS, TURN ON LOCATION',
         "'PŘIPOJIT OVLADAČ (BLUETOOTH)': 'CONNECT CONTROLLER (BLUETOOTH)'" in en and "'NASTAVENÍ BLUETOOTH': 'BLUETOOTH SETTINGS'" in en
         and "'ZAPNOUT POLOHU': 'TURN ON LOCATION'" in en)
kontrola('N1 napoveda CZ i EN: parovani v appce, zapamatovani (vlak = dotyk), povoleni (Zarizeni v okoli / Poloha)',
         'Bluetooth ovladač spáruješ přímo v appce:' in hc and 'Spárování si pamatuje Android' in hc and '„Zařízení v okolí“' in hc
         and 'Pair a Bluetooth controller right in the app:' in he and 'Android remembers the pairing' in he and '“Nearby devices”' in he)
kontrola('V1 build.gradle: versionCode 374, versionName EMU10-B331-BLUETOOTH', 'versionCode 374' in bg and 'versionName "EMU10-B331-BLUETOOTH"' in bg)

print('VYSLEDEK: %d OK, %d CHYBA' % (ok, chyb))
sys.exit(1 if chyb else 0)
