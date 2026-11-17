#!/usr/bin/env python3
"""B335: kontrola zapojeni ve zdrojacich (zvuk YouTube na TV, barvy 4:4:4 v proudu PS1 - Dune carky).
Pouziti: python3 test_overeni/b335/kontrola_b335.py   -> VYSTUP_kontrola_b335.txt
"""
import os
import re
import subprocess
import sys

ZDE = os.path.dirname(os.path.abspath(__file__))
R = os.path.normpath(os.path.join(ZDE, '../..'))


def cti(p):
    return open(os.path.join(R, p), encoding='utf-8').read()


KJ = cti('app/src/main/cpp/tvproud/nap_tv_proud_jpeg.h')
HB = cti('app/src/main/cpp/tvproud/nap_tv_proud.h')
MA = cti('app/src/main/java/eu/atarihelp/emu10/MainActivity.java')
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


# ---- K) kodek PS1: barvy 4:4:4 v malych castech, sousedi z JPEG 4:2:0 ----
over('int plneBarvyDo = 0;' in KJ, 'K1 KodekJpeg.plneBarvyDo vychozi 0 (menu, stranky appky, demo jako drive)')
ps1 = blok(KJ, 'inline void nastavKodekPs1(KodekJpeg &k, int volby) {', '\n}')
menu = blok(KJ, 'inline void nastavKodekMenu(KodekJpeg &k) {', '\n}')
over('static constexpr int PLNE_BARVY_PS1 = 55000;' in KJ and 'k.plneBarvyDo = PLNE_BARVY_PS1;' in ps1 and 'plneBarvyDo' not in menu,
     'K2 nastavKodekPs1: barvy 4:4:4 v castech do 55 000 bodu (jen PS1 - nastavKodekMenu beze zmeny)')
ob = blok(KJ, '  void obdelnik(const uint32_t *F, const Obd &o, int kv, std::vector<uint8_t> &cil) {', '\n  }\n')
over('plne = plneBarvyDo > 0 && plocha <= (size_t)plneBarvyDo;' in ob and 'enc.koduj(F + (size_t)y0 * W + x0, (size_t)W, rw, rh, kv, jpg, casti, plne);' in ob
     and 'hrany420_[(size_t)by * bw + bx] = !s420 ? (uint8_t)0 :' in ob,
     'K3 obdelnik: mala cast 4:4:4 (enc.koduj plneBarvy), JPEG 4:2:0 si zapamatuje hrany ctvercu rozbalene se sousedem')
zk = blok(KJ, '  unsigned zakoduj(const uint32_t *F, int w, int h, int zapisMs, long long ted, std::vector<uint8_t> &out) {', '\n  }\n')
over('if (plneBarvyDo > 0 && doplnSousedy(kop ? &ko : nullptr)) najdiObdelniky(zmena, obdZm);' in zk
     and zk.index('najdiObdelniky(zmena, obdZm);') < zk.index('doplnSousedy(') < zk.index('int kvZmen = q;'),
     'K4 zakoduj: po nalezeni zmenenych casti se doplni sousede ze spolecneho JPEG 4:2:0 (pred kvalitou a kodovanim)')
ds = blok(KJ, '  bool doplnSousedy(const Obd *ko) {', '\n  }\n')
over("if (zmena[i] != 1) continue;" not in ds and 'zmena[i] == 1' in ds and 'zmena[j] = 2;' in ds,
     'K5 doplnSousedy: spousti jen opravdu zmeneny ctverec (doplneny = 2 dal nic nespousti - zadna lavina)')
pp = blok(KJ, '  bool pouzijPosun(', '\n  }\n')
over('hrany420_[(size_t)by * bw + bx] = 0;' in pp, 'K6 kopie (posun): ctverce z kopie hrany 0 (posunuly se i se sousedem - bez doplnovani kazdy snimek)')
over(HB.count('char b[1600];') == 2 and 'char b[1100];' not in HB, 'K7 Hub: radek logu proudu 1600 znaku (info kodeku s radkem B335 se neusekne)')
over('B335: barvy 4:4:4 v castech do %d bodu (casti %lld), ctvercu doplnenych kvuli sousedovi z JPEG 4:2:0 %lld' in KJ,
     'K8 log proudu PS1: "B335: barvy 4:4:4 v castech do 55000 bodu (casti N), ctvercu doplnenych kvuli sousedovi ..."')

# ---- Y) YouTube: zvuk na TV ----
tp = blok(MA, '@JavascriptInterface public boolean tvPosloucha() {', '\n        }')
over('napTvWebRunning && tvSmiZvuk() && (NativeMenuBridge.strankaStavSafe() & 2) != 0' in tp,
     'Y1 AHTVWEB.tvPosloucha(): TV prohlizec pripojeny k proudu stranky (C++ strankaStav bit 2)')
inj = blok(MA, 'private void napTvWebInjectYoutubeAudioBridge(String reason) {', '\n    }\n')
over("window.AHTVWEB.tvPosloucha()" in inj and "document.addEventListener('play',ahtvHraje,true)" in inj and "setInterval(function(){try{var h=ahtvHrajici();" in inj
     and "return 'controls_ready_ORIGINAL_SOUND_untouched';" in inj and "B335_TV_SE_DIVA_zvuk_do_TV:" in inj,
     'Y2 most YouTube: TV se diva -> graf pri prehravani (posluchac play + kontrola 1,5 s), bez TV puvodni zvuk (ORIGINAL FIRST)')
over("if(!ahtvLze(v))return 'skip_cizi_zdroj_original';" in inj and "if(cx.state!=='running')" in inj
     and inj.index("if(cx.state!=='running')") < inj.index('cx.createMediaElementSource(v)'),
     'Y3 most YouTube: cizi zdroj (WebAudio = ticho) se nepripoji; prvek se pripoji az pri bezicim AudioContextu (telefon neztichne)')
over("btn.onclick=function(e){stop(e);var show=panel.style.display==='none';panel.style.display=show?'block':'none';if(show&&window.__ahtvEnsureGraph){window.__ahtvEnsureGraph();}};" in inj,
     'Y4 tlacitko EQ dal stavi graf na pozadani (bez TV jako drive)')
py = blok(MA, '@JavascriptInterface public String pushYoutubePcm16(', '\n        }\n')
over('NativeMenuBridge.strankaZvukSafe(data' in py and '"B335 YOUTUBE ZVUK NA TV: zvuk z YouTube jde do proudu stranky ("' in py,
     'Y5 pushYoutubePcm16: zvuk do proudu stranky (C++), log "B335 YOUTUBE ZVUK NA TV" pri rozjezdu')
try:
    sys.path.insert(0, ZDE)
    import importlib.util
    spec = importlib.util.spec_from_file_location('yt', os.path.join(ZDE, 'test_b335_youtube.py'))
    yt = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(yt)
    r = subprocess.run(['node', '-e', "const a=require('/opt/npm-tools/node_modules/acorn');a.parse(process.argv[1],{ecmaVersion:5});console.log('ok')", yt.JS],
                       capture_output=True, text=True, timeout=60)
    over(r.stdout.strip() == 'ok', 'Y6 skript mostu YouTube (%d znaku) je ES5 (starsi WebView)%s' % (len(yt.JS), '' if r.stdout.strip() == 'ok' else ': ' + r.stderr[-200:]))
except Exception as e:
    over(False, 'Y6 ES5: %s' % e)

# ---- N) napoveda, verze ----
over('YouTube v appce: když se na TV někdo dívá, jde zvuk videa i do televize' in NCZ and 'YouTube in the app: while someone is watching on the TV' in NEN,
     'N1 napoveda CZ / EN: zvuk YouTube na TV')
over('versionCode 378' in GR and 'versionName "EMU10-B335-YT-ZVUK-DUNE"' in GR, 'N2 build.gradle: versionCode 378 "EMU10-B335-YT-ZVUK-DUNE"')

vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (sum(1 for v in vystup if v.startswith(('OK', 'CHYBA'))), chyb))
print('\n'.join(vystup))
open(os.path.join(ZDE, 'VYSTUP_kontrola_b335.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
