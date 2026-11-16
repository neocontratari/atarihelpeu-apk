#!/usr/bin/env python3
"""B333: STRANKY APPKY NA TV Z C++, STARA CESTA PRES JAVU VYPNUTA - kontrola zapojeni ve zdrojacich.
Pouziti (z korene repozitare nebo odkudkoli): python3 test_overeni/b333/kontrola_b333.py -> VYSTUP_kontrola_b333.txt
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
NS = cti('app/src/main/java/eu/atarihelp/emu10/NapTvStranka.java')
NB = cti('app/src/main/java/eu/atarihelp/emu10/NativeMenuBridge.java')
ST = cti('app/src/main/cpp/tvproud/nap_tv_stranka.h')
HU = cti('app/src/main/cpp/tvproud/nap_tv_proud.h')
MG = cti('app/src/main/cpp/menu/nap_menu_gl.cpp')
GR = cti('app/build.gradle')
EN = cti('app/src/main/assets/jazyk/en_tv.js')
NCZ = cti('app/src/main/assets/napoveda/index.html')
NEN = cti('app/src/main/assets/napoveda/en.html')
vystup, chyb = [], 0


def over(ok, co):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)


def stranka_z_javy(src):
    i = src.index('String body = ', src.index('private void napTvWebWriteHtml(OutputStream out)')) + len('String body = ')
    out = []
    while True:
        while src[i] in ' \t\r\n+':
            i += 1
        if src.startswith('//', i):
            i = src.index('\n', i)
            continue
        if src[i] == ';':
            break
        i += 1
        b = []
        while src[i] != '"':
            if src[i] == '\\':
                n = src[i + 1]
                if n == 'u':
                    b.append(chr(int(src[i + 2:i + 6], 16)))
                    i += 6
                    continue
                b.append({'n': '\n', 't': '\t', 'r': '\r', '"': '"', "'": "'", '\\': '\\'}[n])
                i += 2
            else:
                b.append(src[i])
                i += 1
        i += 1
        out.append(''.join(b))
    return ''.join(out)


def funkce(src, hlavicka):
    """telo Java metody od hlavicky po odpovidajici }"""
    i = src.index(hlavicka)
    j = src.index('{', i)
    h, k = 0, j
    while True:
        if src[k] == '{':
            h += 1
        elif src[k] == '}':
            h -= 1
            if h == 0:
                return src[i:k + 1]
        k += 1


HTML = stranka_z_javy(MA)

# ---- S) server TV (Java) ----
over(re.search(r'"/stranka\.stream"\.equals\(path\)\)\s*\{\s*(//[^\n]*\n\s*)?napTvWebJadroProud\(out, s, "STRANKA"\);', MA) is not None,
     'S1 server: /stranka.stream -> napTvWebJadroProud(..., "STRANKA") (proud z C++ jako menu a hry)')
over('static final boolean NAP_TV_STARA_CESTA_VYPNUTA = true;' in MA, 'S2 NAP_TV_STARA_CESTA_VYPNUTA = true (stara cesta pres Javu vypnuta)')
i410 = MA.find('NAP_TV_STARA_CESTA_VYPNUTA && ("/frame.jpg".equals(path)')
ist = [MA.find('else if ("%s".equals(path))' % p) for p in ('/frame.jpg', '/stream.mjpg', '/stream.h264', '/audio.raw')]
over(i410 > 0 and all(x > i410 for x in ist) and '"410 Gone"' in MA[i410:i410 + 700]
     and all(p in MA[i410:i410 + 300] for p in ('/stream.mjpg', '/stream.h264', '/audio.raw')),
     'S3 /frame.jpg, /stream.mjpg, /stream.h264, /audio.raw -> 410 Gone PRED starymi obsluhami (ty uz nic nedostanou)')
k = funkce(MA, 'private boolean napTvWebStaraCestaKouka()')
over('!NAP_TV_STARA_CESTA_VYPNUTA && (' in k, 'S4 napTvWebStaraCestaKouka = false -> Java nefoti obrazovku pro TV, zadny H.264 / JPEG enkoder')
pushe = [funkce(MA, h) for h in ('private void napTvWebAudioPush(', 'private void napTvWebAudioPushMonoPcm16Bytes(', 'private void napTvWebAudioPushStereoPcm16Bytes(')]
over(all('if (NAP_TV_STARA_CESTA_VYPNUTA) return;' in f[:400] for f in pushe), 'S5 zvuk stare cesty (/audio.raw) se neplni: napTvWebAudioPush / Mono / Stereo hned return')
pp = funkce(MA, '@JavascriptInterface public String pushPlayerPcm16(')
py = funkce(MA, '@JavascriptInterface public String pushYoutubePcm16(')
over('NativeMenuBridge.strankaZvukSafe(data, channels <= 1 ? 1 : 2, sampleRate);' in pp and 'NativeMenuBridge.strankaZvukSafe(data, channels <= 1 ? 1 : 2, sampleRate);' in py,
     'S6 zvuk prehravace MP3 a YouTube -> strankaZvukSafe (C++, v paketu /stranka.stream)')
over('if (!NAP_TV_STARA_CESTA_VYPNUTA && atariZarizeni != null' in MA, 'S7 zvuk Atari se pro starou cestu z jadra netaha (jde jen v /atari.stream)')
dr = funkce(MA, 'private String napTvProudDruh()')
over('return j != null ? j : "stranka";' in dr, 'S8 /status: kdyz neni hra / menu / demo, proud=stranka (nikdy proud=0)')
nj = funkce(MA, 'private boolean napTvJadroNaObrazovce(')
over('if ("STRANKA".equals(druh)) return napTvJinyProud() == null;' in nj, 'S9 stranka je "na obrazovce", jen kdyz nema obrazovka vlastni proud z C++')
jp = funkce(MA, 'private void napTvWebJadroProud(OutputStream out, Socket sock, final String druh, final int volby)')
over('if (strankaP) napTvStranka().divak(true);' in jp and 'if (strankaP) napTvStranka().divak(false);' in jp
     and jp.index('klientu.incrementAndGet();') < jp.index('napTvStranka().divak(true)')
     and jp.rindex('} finally {') < jp.index('napTvStranka().divak(false)'),
     'S10 divak stranky: pripojeni -> NapTvStranka foti, odpojeni (finally) -> prestane')
over(all(x in MA for x in ('NativeMenuBridge.strankaOtevriSafe()', 'NativeMenuBridge.strankaDalsiSafe(', 'NativeMenuBridge.strankaZavriSafe(',
                            'NativeMenuBridge.strankaStavSafe()', 'NativeMenuBridge.strankaLogSafe()')),
     'S11 MainActivity: otevri / dalsi / zavri / stav / log proudu "STRANKA" pres NativeMenuBridge')

# ---- J) Java foceni stranky ----
over('new HandlerThread("nap-tv-stranka", Process.THREAD_PRIORITY_DISPLAY)' in NS, 'J1 NapTvStranka: vlastni vlakno "nap-tv-stranka" (ne hlavni)')
over('PixelCopy.request(a.getWindow(), new Rect(0, 0, sw, sh), cil, vysledek -> {' in NS and '}, hh);' in NS,
     'J2 PixelCopy: graficky cip zkopiruje a zmensi okno, vysledek prijde na vlakno stranky (Handler hh)')
over('addOnDrawListener(kresleni)' in NS and 'removeOnDrawListener(kresleni)' in NS and '(spina || od >= KLID_MS)' in NS,
     'J3 foti se jen pri zmene (OnDrawListener) a beze zmeny 2x za sekundu')
over('kde.strankaNaObrazovce()' in NS and 'if (divaku > 0 && !bezi) start();' in NS and 'else if (divaku == 0 && bezi) stop();' in NS,
     'J4 foti jen kdyz se nekdo diva a na obrazovce neni hra / menu / demo')
over('cil.copyPixelsToBuffer(b);' in NS and 'NativeMenuBridge.strankaSnimekSafe(b, cil.getWidth(), cil.getHeight(), cil.getWidth() * 4);' in NS
     and 'ByteBuffer.allocateDirect(' in NS, 'J5 Bitmapa -> prima pamet (ByteBuffer.allocateDirect) -> C++ bez kopie v JNI')
over('if (rgba == null || !rgba.isDirect()' in NB or 'isDirect()' in NB, 'J6 NativeMenuBridge.strankaSnimekSafe: jen prima pamet (jinak nic)')

# ---- C) C++ ----
over('Stranka() : hub_("STRANKA", VZORKOVANI, 9, 16, false)' in ST and 'hub_.nastavKodek(nastavKodekMenu);' in ST and 'hub_.pomerZRozmeru(true);' in ST,
     'C1 nap_tv_stranka.h: Hub "STRANKA" bez orezu, kodek jako menu (zjemneni textu), pomer stran z rozmeru snimku')
over('if (pomerZRozmeru_.load(std::memory_order_relaxed)) pomerStran(W, H, pX, pY);' in HU and HU.count('p8(pk, (unsigned)pX); p8(pk, (unsigned)pY);') == 2
     and 'std::atomic<bool> pomerZRozmeru_{false};' in HU,
     'C2 nap_tv_proud.h: pomer stran z rozmeru PRAVE baleneho snimku (jen stranka, ostatni proudy beze zmeny - vychozi false)')
over('static nap::proud::Stranka &strankaTv()' in MG and MG.index('static nap::proud::Stranka &strankaTv()') < MG.index('extern "C" {'),
     'C3 nap_menu_gl.cpp: strankaTv() pred extern "C" (C++ odkaz), jedna instance na celou appku')
jni = re.findall(r'Java_eu_atarihelp_emu10_NativeMenuBridge_(stranka\w+)\(', MG)
nat = re.findall(r'private static native \S+ (stranka\w+)\(', NB)
over(sorted(jni) == sorted(nat) and len(nat) == 7, 'C4 7 native metod strankaXxx v Jave = 7 funkci JNI v C++: ' + ', '.join(sorted(nat)))

# ---- P) stranka prohlizece na TV ----
over('function startH264(){return;' in HTML, 'P1 stranka TV: startH264 hned konci (zadne H.264 pres Javu)')
over('async function pollAudio(){return;' in HTML, 'P2 stranka TV: pollAudio hned konci (zadne /audio.raw)')
over(re.search(r"window\.napMjStart=function\(\)\{if\(window\.napMjJe\)return;window\.napMjJe=1;if\(window\.napAp&&napAp\.chce\)return;\};", HTML) is not None,
     'P3 stranka TV: napMjStart nic nespousti (zadne /stream.mjpg)')
adresy = re.findall(r'https?://[^\'"\s)]*', HTML)
over(not re.search(r'jsdelivr|unpkg|cdnjs', HTML, re.I) and not adresy,
     'P4 stranka TV: zadna adresa na internet (drive JMuxer z CDN)%s' % ((': ' + ', '.join(adresy)) if adresy else ''))
over('fallback()' in HTML and HTML.count('fallback()') == 1, "P5 stara JPEG smycka fallback() jen definovana, nikde volana")
over("label(fb?'JPEG':'MJPEG')" not in HTML and "label(h264Active?'H264'" not in HTML and 'pollFps calling startH264' not in HTML,
     'P6 popisek nikdy "MJPEG" / "H264"; pollFps uz nevola startH264 (a nespamuje log kazdou vterinu)')
over("else if(napFill&&ap.ow>0&&ap.oh>0&&ap.druh!=='stranka')" in HTML, 'P7 stranka appky se na TV neroztahuje na 16:9 (vyplnit obrazovku jen pro hry)')
over("nd.id='napNeumi'" in HTML and "nd.textContent='TENTO PROHLIZEC NEUMI PROUD Z C++ - OTEVRI ADRESU V CHROME, EDGE NEBO FIREFOXU'" in HTML,
     'P8 prohlizec bez proudu z C++: viditelna hlaska uprostred (cerna obrazovka by nic nerekla)')
over("'TENTO PROHLIZEC NEUMI PROUD Z C++ - OTEVRI ADRESU V CHROME, EDGE NEBO FIREFOXU':" in EN and 'stream from C++' in EN,
     'P9 anglicky: hlaska a stavovy radek prelozene (en_tv.js)')
# syntaxe skriptu stranky TV (acorn, ES2020 - stranka uz drive mela async) a en_tv.js (ES5)
try:
    skripty = re.findall(r'<script(?![^>]*\bsrc=)[^>]*>([\s\S]*?)</script>', HTML, re.I)
    js = ("const a=require('/opt/npm-tools/node_modules/acorn');const fs=require('fs');let ch=0;"
          "const s=JSON.parse(fs.readFileSync(0,'utf8'));"
          "s.tv.forEach((t,i)=>{try{a.parse(t,{ecmaVersion:2020})}catch(e){ch++;console.log('skript '+i+': '+e.message)}});"
          "try{a.parse(s.en,{ecmaVersion:5})}catch(e){ch++;console.log('en_tv.js: '+e.message)}console.log('chyb '+ch);")
    import json
    r = subprocess.run(['node', '-e', js], input=json.dumps({'tv': skripty, 'en': EN}), capture_output=True, text=True, timeout=60)
    over(r.stdout.strip().endswith('chyb 0'), 'P10 syntaxe: %d skriptu stranky TV (acorn) a en_tv.js (ES5) - %s' % (len(skripty), r.stdout.strip().replace('\n', '; ')))
except Exception as e:
    over(False, 'P10 syntaxe skriptu: %s' % e)

# ---- N) napoveda, verze ----
over('OBRAZ: vyplnit obrazovku</span> platí jen pro hry' in NCZ and 'TENTO PROHLIZEC NEUMI PROUD Z C++' in NCZ
     and 'applies to games only' in NEN and 'THIS BROWSER CANNOT PLAY THE STREAM FROM C++' in NEN,
     'N1 napoveda CZ / EN (kapitola WEB TV): stranky cele na vysku, starsi prohlizec -> Chrome / Edge / Firefox')
over('versionCode 376' in GR and 'versionName "EMU10-B333-TV-CPP"' in GR, 'N2 build.gradle: versionCode 376, "EMU10-B333-TV-CPP"')

vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (len(vystup), chyb))
print('\n'.join(vystup))
open(os.path.join(ZDE, 'VYSTUP_kontrola_b333.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
