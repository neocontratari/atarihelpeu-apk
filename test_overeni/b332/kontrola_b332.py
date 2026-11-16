# B332: nazev appky, vic jader pro PS1 (kresleni ve vlakne), zapamatovany Bluetooth ovladac - staticka kontrola
# zdroju (zapojeni, ktere testy na PC neprojdou: Android, CMake, manifest).
# Spusteni z test_overeni/b332: python3 kontrola_b332.py  -> VYSTUP_kontrola_b332.txt
import io, os, re, sys
import xml.dom.minidom as md

KOREN = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
ok = chyb = 0
vystup = []


def cti(p):
    return io.open(os.path.join(KOREN, p), encoding='utf-8').read()


def kontrola(co, v):
    global ok, chyb
    vystup.append(('OK     ' if v else 'CHYBA  ') + co)
    print(vystup[-1])
    if v:
        ok += 1
    else:
        chyb += 1


mf = cti('app/src/main/AndroidManifest.xml')
bg = cti('app/build.gradle')
nat = cti('app/src/main/cpp/nap_ps1_native.cpp')
thr = cti('app/src/main/cpp/vendor/pcsx_rearmed/frontend/pcsxr-threads.c')
most = cti('app/src/main/cpp/vendor/pcsx_rearmed/plugins/gpu_neon/nap_most.c')
lr = cti('app/src/main/cpp/vendor/pcsx_rearmed/frontend/libretro.c')
cm = cti('app/src/main/cpp/CMakeLists.txt')
bt = cti('app/src/main/java/eu/atarihelp/emu10/NapBluetooth.java')
ma = cti('app/src/main/java/eu/atarihelp/emu10/MainActivity.java')
ix = cti('app/src/main/assets/index.html')
hc = cti('app/src/main/assets/napoveda/index.html')
he = cti('app/src/main/assets/napoveda/en.html')

# ---- nazev a verze ----
d = md.parseString(mf.encode('utf-8'))
app = d.getElementsByTagName('application')[0]
kontrola('N1 nazev appky pod ikonou: "%s" (Rene: "(C)N&P Vision")' % app.getAttribute('android:label'),
         app.getAttribute('android:label') == '©N&P Vision' and 'android:label="AtariHelp TEST"' not in mf)
kontrola('N2 balicek se nemeni (aktualizace pres starou appku): applicationId eu.atarihelp.emu10.test',
         re.search(r'applicationId\s+"eu\.atarihelp\.emu10\.test"', bg) is not None)
kontrola('V1 build.gradle: versionCode 375, versionName EMU10-B332-VISION', 'versionCode 375' in bg and 'versionName "EMU10-B332-VISION"' in bg)

# ---- PS1: kresleni ve vlakne ----
fn = nat[nat.index('static const char *nap_core_option_value('):nat.index('static bool nap_env(')]
kontrola('P1 jadro dostane odpoved: kresleni ve vlakne "auto" (jako RetroArch), zvuk ve vlakne "disabled", prekladac "auto"',
         'if (strcmp(key, "pcsx_rearmed_gpu_thread_rendering") == 0) return g_ps1_kresleni_volba;' in fn
         and 'static const char *g_ps1_kresleni_volba = "auto";' in nat
         and 'if (strcmp(key, "pcsx_rearmed_spu_thread") == 0) return "disabled";' in fn
         and 'if (strcmp(key, "pcsx_rearmed_drc_thread") == 0) return "auto";' in fn)
kontrola('P2 jadro ma kresleni ve vlakne prelozene (CMake: USE_ASYNC_GPU, gpulib/gpu_async.c v napps1core)',
         'USE_ASYNC_GPU' in cm and '${PS1_DIR}/plugins/gpulib/gpu_async.c' in cm[cm.index('add_library(napps1core SHARED'):])
ob = thr[thr.index('static void nap_vlakno_obal_fn(void *p)'):]
kontrola('P3 vlakna jadra hlasi start a konec (pcsxr-threads.c): obal kolem funkce vlakna, puvodni funkce se stejnym argumentem',
         'void __attribute__((weak)) nap_pcsxr_vlakno(int typ, int zacatek)' in thr
         and re.search(r'nap_pcsxr_vlakno\(o\.typ, 1\);\s*o\.fn\(NULL\);\s*nap_pcsxr_vlakno\(o\.typ, 0\);', ob) is not None
         and 'h = sthread_create(nap_vlakno_obal_fn, o);' in thr and 'h = sthread_create(thread_func, NULL);' in thr)
kontrola('P4 appka vlakna prijme (silna funkce prepise "weak" z jadra) a pamatuje TID podle typu',
         'extern "C" void nap_pcsxr_vlakno(int typ, int zacatek) {' in nat and 'g_ps1_tid_vlakna[typ].store(tid);' in nat
         and 'g_ps1_tid_vlakna[typ].compare_exchange_strong(t, 0);' in nat)
cf = nat[nat.index('static void nap_core_thread_fn(void) {'):]
cf = cf[:cf.index('\n}\n')]
kontrola('P5 vlakno emulace: kreslici vlakno pripne na ostatni rychla jadra (start, nove vlakno, kontrola kazde ~2 s)',
         'ps1_pripni_kresleni(tidKresleni, maskaKresleni, g_ps1_pripnute_jadro, "pri startu")' in cf
         and 'ps1_pripni_kresleni(tk, maskaKresleni, g_ps1_pripnute_jadro, "nove vlakno")' in cf
         and '!ps1_tid_ma_masku(tidKresleni, maskaKresleni)' in cf
         and cf.index('g_ps1_pripnute_jadro = ps1_pripni_emulaci(') < cf.index('ps1_maska_kresleni(g_ps1_cpu_koren, g_ps1_pripnute_jadro)'))
kontrola('P6 radek VYKON: cas procesoru kresliciho vlakna na snimek a jeho jadra; v deniku VLAKNA',
         '" | kresleni ve vlakne %.1f ms na snimek%s, jadra %s | takt kresleni: %s"' in cf and 'ps1_vlakna_denik();' in cf
         and 'kresleniNsOd = ps1_cas_vlakna_ns(tidKresleni);   // B332: nove okno' in cf)
kontrola('P7 skutecny stav kresleni ve vlakne z gpulib (gpu.async) a zvuk ve vlakne v NASTAVENI GRAFIKY',
         'int nap_ps1_kresleni_ve_vlakne(void)' in most and 'return gpu.async != NULL;' in most
         and '"kresleni ve vlakne %s, zvuk ve vlakne %s | preskakovani snimku %d"' in lr and 'spu_config.iUseThread ? "ANO" : "NE"' in lr)
kontrola('P8 2x (Enhanced Resolution) zustava zapnute', 'if (strcmp(key, "pcsx_rearmed_neon_enhancement_enable") == 0) return "enabled";' in fn)

# ---- Bluetooth: zapamatovany ovladac ----
kontrola('B1 hlidac zapamatovanych ovladacu od startu appky (ACL / HID / Bluetooth zap-vyp), Android 13+ s priznakem RECEIVER_EXPORTED',
         'try { spustHlidac(); }' in bt and 'f.addAction(BluetoothDevice.ACTION_ACL_CONNECTED);' in bt and 'f.addAction(AKCE_HID);' in bt
         and 'if (Build.VERSION.SDK_INT >= 33) a.registerReceiver(hlidac, f, 2 /* Context.RECEIVER_EXPORTED */);' in bt
         and 'static final String AKCE_HID = "android.bluetooth.input.profile.action.CONNECTION_STATE_CHANGED";' in bt)
kontrola('B2 konec appky: hledani zastavit, hlidac odhlasit, HID profil zavrit (onDestroy: zastav() a konec())',
         'try { if (bt != null) bt.zastav(); }' in ma and 'try { if (bt != null) bt.konec(); }' in ma
         and ma.index('try { if (bt != null) bt.zastav(); }') < ma.index('try { if (bt != null) bt.konec(); }')
         and 'a.unregisterReceiver(hlidac);' in bt and 'ad.closeProfileProxy(PROFIL_HID, p);' in bt)
kontrola('B3 stav ovladace z verejneho HID profilu Androidu (getProfileProxy 4, getConnectionState)',
         'ad.getProfileProxy(a, new BluetoothProfile.ServiceListener()' in bt and 'static final int PROFIL_HID = 4;' in bt
         and 'return h.getConnectionState(d);' in bt)
kontrola('B4 sama pripojit (skryte connect) jen Android 7 a 8 - od 9 to Android aplikacim nedovoli, nic se neobchazi',
         'NapBluetoothTexty.appkaSmiPripojit(Build.VERSION.SDK_INT)' in bt and 'getMethod("connect", BluetoothDevice.class)' in bt
         and bt.count('getMethod(') == 1 and 'setAccessible' not in bt and 'VMRuntime' not in bt and 'getDeclaredMethod' not in bt)
pu = bt[bt.index('private void pripojUi(String adresa)'):bt.index('private String pripojSkryte(')]
kontrola('B5 PRIPOJIT: Android 12+ povoleni BLUETOOTH_CONNECT, vypnute Bluetooth -> systemove okno, pak pokracuje (povoleniVysledek / zapnutiVysledek)',
         'a.requestPermissions(new String[]{"android.permission.BLUETOOTH_CONNECT"}, REQ_POVOLENI);' in pu
         and 'new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE), REQ_ZAPNUTI' in pu
         and bt.count('if (vse && p != null) pripojUi(p);') == 1 and bt.count('if (zapnuto && p != null) pripojUi(p);') == 1)
hl = bt[bt.index('private final BroadcastReceiver hlidac'):bt.index('private void kontrolaOzvani(')]
kontrola('B6 do logu jen zapamatovane ovladace a konec adresy (cizi zarizeni v okoli ne)',
         'if (d == null || !zapamatovanyOvladac(d)) return;' in hl and 'NapBluetoothTexty.kratkaAdresa(adr)' in hl and 'getAddress() +' not in hl)
kontrola('B7 "ozval se, ale nepripojil": kontrola 10 s po ozvani, zprava jednou za ozvani',
         'ui.postDelayed(() -> kontrolaOzvani(d), NapBluetoothTexty.LIMIT_OZVAL_MS + 500L);' in bt and 'ozvalHlaseno.add(adr);' in bt
         and 'NapBluetoothTexty.zpravaOzvalSe(en(), jm)' in bt)
kontrola('B8 JS rozhrani AHBT.pripoj(adresa)', '@JavascriptInterface public void pripoj(String adresa) { if (bt != null) bt.pripoj(adresa); }' in ma)
b332 = ix[ix.index('// B332: u zapamatovaneho ovladace'):ix.index('function btZavri(){')]
kontrola('S1 OPTIONS: PRIPOJIT „jmeno“ u zapamatovaneho nepripojeneho (data-nepreklad, jen textContent), starsi appka bez pripoj bez chyby',
         '<div id="stBtZapamSeznam" data-nepreklad></div>' in ix and 'innerHTML' not in b332 and 'b.textContent=' in b332
         and 'if(window.AHBT&&AHBT.pripoj) AHBT.pripoj(a);' in b332)
kontrola('H1 napoveda CZ i EN: PRIPOJIT u zapamatovaneho, co znamenaji vysledky, proc appka sama nepripoji (Android 9+)',
         'Zapamatovaný ovladač se nepřipojil?' in hc and 'od Androidu 9 jen Nastavení Androidu' in hc
         and 'A remembered controller didn’t connect?'.replace('’', "'") in he and 'Since Android 9 only the Android settings can connect' in he)

vystup.append('VYSLEDEK: %d OK, %d CHYBA' % (ok, chyb))
print(vystup[-1])
io.open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'VYSTUP_kontrola_b332.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
