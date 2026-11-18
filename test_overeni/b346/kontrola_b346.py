#!/usr/bin/env python3
"""B346: zapojeni ve zdrojich appky (staticka kontrola).

  D1-D3) vyvojarska tlacitka pryc (Rene: "ano vyvojarske tlacitka smaz"): panel PS1 bez PS1 PRIMA URL, PS1 CACHE
         CESTA, SMAZAT PS1 CACHE (+ jejich JS, preklady, mosty ps1RemoteCachePath / ps1ClearRemoteCache a
         clearPs1RemoteCaches); most ps1DownloadAndBoot ZUSTAVA (web atarihelp.eu); OPTIONS bez TV POKUS (+ most
         AHINTRO.tvPokus, prepinac tvPokusRezim a jeho podminky v TV prenosu, preklady)
  O1-O4) poskozeny obraz disku (C&C): cisty C++ nap_ps1_obraz.h, denik CUE / .bin kontroluje cele sektory, odmitnuta
         hra s vadou -> "PS1_HRA_FAIL obraz"; Java veta NapPravidla.ps1ProcNespustila v obou cestach startu;
         rozbaleny soubor = velikost z ZIPu
  S1)    stranka PS1: chybu spusteni rekne jednou (hlaska okna, jinak pruh), dlouha hlaska dele
  N1-N2) napoveda CZ / EN: veta o poskozenem obrazu, TV POKUS / TV TEST pryc, stejna stavba
  V)     verze 389 "EMU10-B346-UKLID-OBRAZ-DISKU"
Pouziti (z test_overeni/b346): python3 kontrola_b346.py -> VYSTUP_kontrola_b346.txt
"""
import os
import re

ZDE = os.path.dirname(os.path.abspath(__file__))
KOREN = os.path.normpath(os.path.join(ZDE, '../..'))
M = os.path.join(KOREN, 'app/src/main')
J = os.path.join(M, 'java/eu/atarihelp/emu10')
A = os.path.join(M, 'assets')
C = os.path.join(M, 'cpp')
vystup = []
chyb = 0


def over(ok, co):
    global chyb
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)
    print(vystup[-1], flush=True)
    if not ok:
        chyb += 1


def cti(*c):
    try:
        with open(os.path.join(*c), encoding='utf-8') as f:
            return f.read()
    except OSError:
        return ''


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


MA = cti(J, 'MainActivity.java')
NP = cti(J, 'NapPravidla.java')
PS = cti(A, 'emu_ps1/index.html')
PS_KOD = re.sub(r'<!--[\s\S]*?-->', '', re.sub(r'/\*[\s\S]*?\*/', '', PS))   # bez komentaru (B346 v nich vysvetluje, co je pryc)
IX = cti(A, 'index.html')
ENP = cti(A, 'jazyk/en_ps1.js')
ENM = cti(A, 'jazyk/en_menu.js')
NAT = cti(C, 'nap_ps1_native.cpp')
OBR = cti(C, 'nap_ps1_obraz.h')

# ---------------- D) vyvojarska tlacitka ----------------
over(not any(x in PS_KOD for x in ('psRemoteUrl', 'psCachePath', 'psCacheClear', 'bootPs1RemoteUrl', 'ps1ShowCachePath',
                               'ps1ClearCache(', 'ps1NormalizeRemoteUrl', 'ps1RemoteCachePath', 'ps1ClearRemoteCache',
                               'PS1 PRIMA URL', 'PS1 CACHE CESTA', 'SMAZAT PS1 CACHE'))
     and 'id="psGooglePage"' in PS and 'id="tvWebCast"' in PS and 'id="logSave"' in PS and 'id="logCopy"' in PS,
     'D1) panel PS1: PS1 PRIMA URL / PS1 CACHE CESTA / SMAZAT PS1 CACHE pryc i s JS, zbytek panelu zustal')
over(not any(x in ENP for x in ("'PS1 PRIMA URL'", "'PS1 CACHE CESTA'", "'SMAZAT PS1 CACHE'", 'PS1 prima URL', 'PS1 CACHE cesta chyba',
                                'Smazat PS1 stazene hry'))
     and 'public String ps1RemoteCachePath()' not in MA and 'public String ps1ClearRemoteCache()' not in MA
     and 'clearPs1RemoteCaches' not in MA and 'public void ps1DownloadAndBoot(String url)' in MA,
     'D2) preklady a mosty tlacitek pryc (ps1RemoteCachePath, ps1ClearRemoteCache), ps1DownloadAndBoot pro web zustal')
over('stTvPokus' not in IX and 'TV POKUS' not in IX.replace('// B346: OPTIONS -> TV POKUS', '') and 'tvPokus(' not in MA
     and 'tvPokusRezim' not in MA and 'tvSmiZvuk' not in MA and 'tvSmiObraz' not in MA and 'TV_WEB_AUDIO_VYPNUT_POKUSEM' not in MA
     and 'TV POKUS' not in ENM and 'return napTvWebRunning && (NativeMenuBridge.strankaStavSafe() & 2) != 0;' in MA,
     'D3) OPTIONS -> TV POKUS pryc: tlacitko, JS, most AHINTRO.tvPokus, prepinac a jeho podminky v TV prenosu, preklady')

# ---------------- O) poskozeny obraz ----------------
over('inline int sektor_stopy(const char *radek)' in OBR and 'inline std::string vada(const std::string &jmeno, long long velikost, int sektor)' in OBR
     and 'inline std::string vada_bez_cue(' in OBR and '#include <jni.h>' not in OBR and 'android' not in OBR.lower().replace('bez androidu', ''),
     'O1) nap_ps1_obraz.h: cisty C++ (bez JNI / Androidu) - velikost sektoru stopy, vada velikosti, .bin bez .cue')
den = metoda(NAT, 'static void ps1_denik_cue(const std::string &cue)')
st = metoda(NAT, 'static void ps1_denik_start(const std::string &hra, const std::string &sys)')
over('#include "nap_ps1_obraz.h"' in NAT and 'nap_obraz::vada(souborJm, souborVel, nap_obraz::sektor_stopy(s))' in den
     and 'g_obraz_vada.clear();' in st and 'nap_obraz::vada_bez_cue(' in st
     and 'if (jeHra && !g_obraz_vada.empty()) {' in NAT and 'return env->NewStringUTF("PS1_HRA_FAIL obraz");' in NAT,
     'O2) nap_ps1_native.cpp: denik kontroluje cele sektory (CUE i .bin bez .cue), odmitnuta hra s vadou -> PS1_HRA_FAIL obraz')
boot = metoda(MA, 'private void bootPs1FileOnCurrentThread(File gameFile, String label, String reason)')
over('static String ps1ProcNespustila(String r, boolean en)' in NP and 'NapPravidla.ps1ProcNespustila(r, NapJazyk.en(MainActivity.this))' in boot
     and 'NapPravidla.ps1ProcNespustila(ps1LastBootResult, NapJazyk.en(MainActivity.this))' in MA
     and 'CHYBA jadro hru nespustilo' not in MA,
     'O3) Java: veta pro hrace NapPravidla.ps1ProcNespustila v obou cestach startu (stazena hra / knihovna i vyber souboru)')
ex = metoda(MA, 'private File extractZipEntryToFile(java.util.zip.ZipFile zip, java.util.zip.ZipEntry entry, File dir, String safeName)')
over('long vZipu = entry.getSize();' in ex and 'if (vZipu >= 0 && vZipu != total) {' in ex and '" (v ZIPu " + vZipu + ")"' in ex,
     'O4) rozbaleni ZIPu: soubor musi mit velikost z ZIPu (jinak "ZIP je poškozený"), log ukaze obe velikosti')

# ---------------- S) stranka ----------------
over('spusteni:!!m[1]' in PS and 'if(ch.spusteni&&(NAHR.on||Date.now()-(window.__napChybaToastem||0)<5000))return;' in PS
     and 'Math.max(7000,Math.min(15000,zprava.length*60))' in PS,
     'S1) stranka PS1: chybu spusteni rekne jednou (hlaska okna, jinak pruh), dlouha hlaska az 15 s')

# ---------------- N) napoveda ----------------
cz, en = cti(A, 'napoveda/index.html'), cti(A, 'napoveda/en.html')
over('napíše <b>obraz disku je poškozený</b>, soubor hry nemá celé sektory CD' in cz
     and 'says <b>the disc image is damaged</b>, the game file does not have whole CD sectors' in en
     and 'TV POKUS' not in cz and 'TV TEST' not in en,
     'N1) napoveda CZ / EN: veta o poskozenem obrazu, TV POKUS / TV TEST pryc')


def stavba(t):
    t = re.sub(r'<!--[\s\S]*?-->', '', t)
    return re.findall(r'<(h[1-4]|li|dt|dd|p|section|ul|ol|dl)\b', t)


over(stavba(cz) == stavba(en), 'N2) napoveda CZ a EN maji stejnou stavbu (%d prvku)' % len(stavba(cz)))

# ---------------- V) verze ----------------
g = cti(KOREN, 'app/build.gradle')
over('versionCode 389' in g and 'versionName "EMU10-B346-UKLID-OBRAZ-DISKU"' in g, 'V) verze 389 "EMU10-B346-UKLID-OBRAZ-DISKU"')

vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb))
print(vystup[-1])
with open(os.path.join(ZDE, 'VYSTUP_kontrola_b346.txt'), 'w', encoding='utf-8') as f:
    f.write('\n'.join(vystup) + '\n')
raise SystemExit(1 if chyb else 0)
