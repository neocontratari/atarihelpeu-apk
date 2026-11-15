# B329: zapojeni v kodu appky (staticka kontrola zdroju) - co testy na PC neprojdou (Android, MainActivity).
# Spusteni z test_overeni/b329: python3 kontrola_b329.py  -> VYSTUP_kontrola_b329.txt
import io, os, re, sys

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
no = cti('app/src/main/java/eu/atarihelp/emu10/NapOvladac.java')
na = cti('app/src/main/java/eu/atarihelp/emu10/NapAktualizace.java')
nn = cti('app/src/main/cpp/nap_ps1_native.cpp')
bg = cti('app/build.gradle')
cz = cti('app/src/main/assets/napoveda/index.html')
en = cti('app/src/main/assets/napoveda/en.html')

# ---- 1) Pro Controller podle mista ----
ok_kl = ma[ma.index('private boolean ovladacKlavesa(KeyEvent e)'):ma.index('private int ovladacMenu(')]
kontrola('O1 MainActivity.ovladacKlavesa: kod podle mista (ovladac.kodPodleMista) PRED rozeslanim do PS1 / Segy / Atari / menu',
         'final int k = ovladac != null ? ovladac.kodPodleMista(e) : e.getKeyCode();' in ok_kl
         and ok_kl.index('kodPodleMista') < ok_kl.index('NativePs1CoreBridge.ovladacKlavesaSafe(k, dolu)')
         and 'NativeSegaCoreBridge.ovladacKlavesaSafe(k, dolu)' in ok_kl and 'NativeAtariCoreBridge.devOvladacKlavesaSafe(k, dolu)' in ok_kl
         and 'ovladacMenu(k, dolu, e.getRepeatCount())' in ok_kl)
kontrola('O2 log: ovladac.klavesa(e, k, ...) - puvodni kod i preklad', 'ovladac.klavesa(e, k, OVL_KAM[kam], t);' in ok_kl)
pz = no[no.index('private Zarizeni poznej(int id, String proc)'):no.index('private void prepocitej(String proc)')]
kontrola('O3 NapOvladac.poznej: vyrobce / produkt + tlacitko C -> switchStara; v logu pri pripojeni "stara tabulka" a tlacitka C / Z',
         'z.switchStara = switchStaraTabulka(vyrobce, produkt, ma[I_TLACITKO_C]);' in pz and 'Pro Controller se STAROU tabulkou' in pz
         and 'KeyEvent.KEYCODE_BUTTON_C, KeyEvent.KEYCODE_BUTTON_Z};' in no
         and no.count('"DPAD", "BACK", "C", "Z"}') == 1 and 'I_TLACITKO_C = 15' in no)
kontrola('O4 zmena zarizeni (baterie apod.) neztrati, ktera tlacitka uz sla do logu', 'z.hlaseno = byl.hlaseno;' in pz)
kl = no[no.index('void klavesa(KeyEvent e, int kod, String kam, int t)'):no.index('private void ovladacHraje(Zarizeni z)')]
kontrola('O5 kazde tlacitko kazdeho ovladace POPRVE do logu (do 40 ruznych), stary strop 12 / 10 stisku pryc',
         'h.size() < MAX_TLACITEK_LOG && h.add(puvodni)' in kl and 'MAX_TLACITEK_LOG = 40' in no
         and 'prvnichKlaves' not in no and 'neznamychKlaves' not in no and 'B329 OVLADAC tlacitko' in kl)

# ---- 2) menu na pozadi ----
mk = ma[ma.index('private void menuKontrola()'):ma.index('/** Vysledek dotyku / klavesy z C++')]
kontrola('M1 hlidac menu: na pozadi (napViditelna false) nehlida a po navratu pocita 8 s znovu',
         'if (!napViditelna) {' in mk and mk.index('if (!napViditelna)') < mk.index('menuNelze = true')
         and 'menuStartMs = 0;' in mk and 'if (menuStartMs == 0) menuStartMs = System.currentTimeMillis();' in mk)

# ---- 3) Aktualizace hotova do OK ----
ps = na[na.index('public static String poStartu(final Activity a)'):na.index('public static void oznamNovouVerzi(')]
kontrola('A1 poStartu: oznameni jen ulozi jako cekajici (zadne okno 1,5 s po startu)',
         'sp.edit().putInt(K_OZNAMIT, pi.versionCode).putInt(K_OZNAMIT_UKAZANO, 0).apply();' in ps
         and 'postDelayed' not in ps and 'new AlertDialog' not in ps)
oz = na[na.index('public static void oznamNovouVerzi('):na.index('public static void zavriOznameni()')]
kontrola('A2 oznamNovouVerzi: jen OK zavre (setCancelable false, ne dotyk mimo), OK smaze cekajici, pocita ukazani (NapAktualizaceTexty.oznameni)',
         '.setCancelable(false)' in oz and 'd.setCanceledOnTouchOutside(false);' in oz
         and '.remove(K_OZNAMIT).remove(K_OZNAMIT_UKAZANO)' in oz and 'NapAktualizaceTexty.oznameni(ceka, pi.versionCode, ukazano)' in oz
         and 'putInt(K_OZNAMIT_UKAZANO, ukazano + 1)' in oz)
nv = ma[ma.index('private void napOznamVerzi(String proc)'):]
nv = nv[:nv.index('}') + 1]
kontrola('A3 MainActivity.napOznamVerzi: az po intru a volbe jazyka, jen kdyz je appka videt',
         'demoPlocha != null || napCekaNaJazyk || !napViditelna' in nv and 'NapAktualizace.oznamNovouVerzi(this, proc, this::appendNativeLog)' in nv)
kontrola('A4 volano: po intru (i kdyz C++ menu nejde), pri ukazani menu, pri navratu do appky; konec aktivity okno zavre',
         'napOznamVerzi("po intru");' in ma and 'napOznamVerzi("menu " + proc);' in ma and 'napOznamVerzi("navrat do appky");' in ma
         and 'NapAktualizace.zavriOznameni();' in ma[ma.index('protected void onDestroy()'):ma.index('protected void onResume()')])

# ---- 4) PS1 na jednom jadre ----
vl = nn[nn.index('static void nap_core_thread_fn(void)'):]
kontrola('P1 vlakno emulace PS1 se pri startu pripne na nejrychlejsi jadro (ps1_pripni_emulaci)',
         'g_ps1_pripnute_jadro = ps1_pripni_emulaci(g_ps1_cpu_koren);' in vl[:4000])
kontrola('P2 v behu kontrola zhruba kazde 2 s (120 kroku): Android zmenil jadra -> pripnout znovu',
         'if (g_ps1_pripnute_jadro >= 0 && ++pripnutiKontrola >= 120)' in vl and '!ps1_vlakno_pripnute(g_ps1_pripnute_jadro)' in vl)
kontrola('P3 pripina se jen na telefonu s ruzne rychlymi jadry; chyba pripnuti = emulace bezi jako drive',
         'if (j.jader < 2 || j.minKhz >= j.maxKhz)' in nn and 'emulace bezi jako drive' in nn)
kontrola('P4 radek VYKON: takt jadra (ted / smi / umi, governor) a "(pripnuto)"',
         '| takt: %s%s | TV divaku %d"' in vl and 'ps1_takt_text(g_ps1_cpu_koren, taktCpu)' in vl)
kontrola('P5 hra se nemeni: pripnuti jen sched_setaffinity volajiciho vlakna (zadna zmena jadra PS1 ani libretro.c v B329)',
         'syscall(__NR_sched_setaffinity, 0, sizeof(m), m)' in nn)

# ---- 5) napoveda a verze ----
kontrola('N1 napoveda CZ i EN: Pro Controller na Androidu 9 se preklada podle mista; po aktualizaci okno Aktualizace hotova / Update done -> OK',
         'hlásí jako <b>Pro Controller</b>' in cz and 'report themselves as a <b>Pro Controller</b>' in en
         and cz.count('Aktualizace hotová</span>') == 2 and en.count('Update done</span>') == 2)
kontrola('V1 build.gradle: versionCode 372, versionName EMU10-B329-OVLADAC', 'versionCode 372' in bg and 'versionName "EMU10-B329-OVLADAC"' in bg)

print('VYSLEDEK: %d OK, %d CHYBA' % (ok, chyb))
sys.exit(1 if chyb else 0)
