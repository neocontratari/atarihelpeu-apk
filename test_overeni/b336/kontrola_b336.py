#!/usr/bin/env python3
"""B336: kontrola zapojeni (zvuk YouTube bez chrapotu, menu pristroje Atari na TV, log predchoziho spusteni).
Pouziti: python3 test_overeni/b336/kontrola_b336.py   -> VYSTUP_kontrola_b336.txt
"""
import importlib.util
import os
import subprocess
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
R = os.path.normpath(os.path.join(ZDE, '../..'))


def cti(p):
    return open(os.path.join(R, p), encoding='utf-8').read()


MA = cti('app/src/main/java/eu/atarihelp/emu10/MainActivity.java')
HB = cti('app/src/main/cpp/tvproud/nap_tv_proud.h')
ST = cti('app/src/main/cpp/tvproud/nap_tv_stranka.h')
GR = cti('app/build.gradle')
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


# ---- Y) zvuk YouTube ----
inj = blok(MA, 'private void napTvWebInjectYoutubeAudioBridge(String reason) {', '\n    }\n')
over("new MediaStreamTrackProcessor({track:dest.stream.getAudioTracks()[0],maxBufferSize:400})" in inj and "gain.connect(dest);" in inj
     and "createScriptProcessor(2048" not in inj,
     'Y1 odbocka: MediaStreamTrackProcessor (fronta az 400 kousku ~4 s - zadrhnuti stranky nic neztrati), ScriptProcessor 2048 pryc')
over("if(!zachyt){tap=ctx.createScriptProcessor(4096,2,2);" in inj and "zachyt='sp4096'" in inj,
     'Y2 zaloha bez MediaStreamTrackProcessor (WebView pred Chrome 94): ScriptProcessor 4096')
over("if(!q||!ahtvHrajici()){aN=0;return;}" in inj and "peak<0.0005" not in inj and "var n=Math.min(8,q.length);" in inj
     and "if(q.length>256)" in inj,
     'Y3 posila se cely zvuk (i ticho), dokud neco hraje; fronta po 8 kusech (dozene zadrhnuti), zahazuje se az nad 256 kusu (~11 s)')
over("ahtvLog6('zachyt zvuku pro TV: '" in inj and "ahtvLog6('zachyt '+(bz&&bz.zachyt?bz.zachyt:'?')+': za '" in inj,
     'Y4 log JSLOG "B336 YOUTUBE ZVUK NA TV": cesta zachytu a kazdych 30 s kusu, vzorku/s, fronta, zahozeno')
try:
    spec = importlib.util.spec_from_file_location('t336', os.path.join(ZDE, 'test_b336_youtube_zvuk.py'))
    js = None
    src = MA
    # stejne vytazeni jako v testu (bez spousteni testu)
    sys.argv = [sys.argv[0], os.path.join(R, 'app/src/main/java/eu/atarihelp/emu10/MainActivity.java')]
    t = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(t)
    r = subprocess.run(['node', '-e', "const a=require('/opt/npm-tools/node_modules/acorn');a.parse(process.argv[1],{ecmaVersion:5});console.log('ok')", t.JS336],
                       capture_output=True, text=True, timeout=60)
    over(r.stdout.strip() == 'ok', 'Y5 skript mostu YouTube (%d znaku) je ES5%s' % (len(t.JS336), '' if r.stdout.strip() == 'ok' else ': ' + r.stderr[-200:]))
except Exception as e:
    over(False, 'Y5 ES5: %s' % e)

# ---- H) Hub / stranka TV ----
over('std::atomic<int> zvukMaxMs_{250};' in HB and '(unsigned long long)zvukMaxMs_.load(std::memory_order_relaxed) / 1000;' in HB
     and 'hub_.zvukMaxMs(600);' in ST and HB.count('zvukMaxMs(') == 1,
     'H1 Hub: zvuk mezi pakety vychozi 250 ms (PS1, menu, Sega jako drive), proud stranky 600 ms (davka YouTube po zadrhnuti)')
zv = blok(HB, '  void zvukMaxMs(int ms) {', '\n  }')
over('ZVUK_KAP * 1000' in zv and '- 50' in zv, 'H2 zvukMaxMs se omezi pod velikost kruhu zvuku (ZVUK_KAP)')
over("else if(lb>(ap.druh==='stranka'?0.7:0.35)||" in MA, 'H3 stranka TV: rezerva zvuku stranky az 0,7 s (davka se nezahodi), hry dal 0,35 s')

# ---- A) Atari na TV ----
bp = blok(MA, '    public void onBackPressed() {', '\n    }\n')
over(bp.index('atariZarizeniSkryj(false);') < bp.index('if (atariZarizeni != null && atariCppNaObrazovce && napJeTv()) {') < bp.index('if (menuPlocha != null) { menuDoPozadi(); return; }')
     and 'atariCppTvMenu();' in bp,
     'A1 ZPET na TV v pristroji Atari -> menu pristroje (po navratu ze stranky s logem, pred menu / historii)')
tm = blok(MA, '    private void atariCppTvMenu() {', '\n    }\n')
over(all(x in tm for x in ('case 0: atariZarizeniAkce(1);', 'case 1: atariZarizeniAkce(2);', 'case 2: atariZarizeniAkce(3);', 'case 3: atariZarizeniAkce(9);',
                           'case 4: atariZarizeniAkce(4);', 'case 5: NativeAtariCoreBridge.devCtlSafe("atari_reset");', 'case 6: atariZarizeniAkce(7);',
                           'default: atariZarizeniAkce(8);')) and 'B336 TV: ZPET -> menu pristroje Atari' in tm,
     'A2 menu pristroje na TV: NET / HRY, XEX, ATR / DISK, KAZETA, TURBO BASIC, RESET, NAPOVEDA, ZPET DO MENU (stejne akce jako tlacitka na vysku)')

# ---- L) log ----
lg = blok(MA, '} else if ("/log/predchozi".equals(path)) {', '} else if ("/log".equals(path)) {')
over('new File(napTvWebLogSlozka, "PS1_LOG_predchozi.txt")' in lg, 'L1 WEB TV /log/predchozi = log predchoziho spusteni (Download/AtariHelp/PS1_LOG_predchozi.txt)')

# ---- N) napoveda, verze ----
over('<b>Atari na TV:</b> tlačítko <span class="t">ZPĚT</span> otevře menu přístroje' in NCZ and '<b>Atari on the TV:</b> the <span class="t">BACK</span> button opens the device menu' in NEN
     and 'SEGA a PS1 se na novém zařízení otevírají stejně jako na telefonu' in NCZ, 'N1 napoveda CZ / EN: menu pristroje Atari na TV, SEGA / PS1 na novem zarizeni')
over('versionCode 379' in GR and 'versionName "EMU10-B336-YT-BEZ-CHRAPOTU-TV"' in GR, 'N2 build.gradle: versionCode 379 "EMU10-B336-YT-BEZ-CHRAPOTU-TV"')

vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (sum(1 for v in vystup if v.startswith(('OK', 'CHYBA'))), chyb))
print('\n'.join(vystup))
open(os.path.join(ZDE, 'VYSTUP_kontrola_b336.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
