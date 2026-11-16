#!/usr/bin/env python3
"""B334: ANDROID TV - kontrola zapojeni ve zdrojacich (32bitove knihovny, nabidka TV, dalkovy ovladac).
Pouziti: python3 test_overeni/b334/kontrola_b334.py  [ANDROID_JAR]  -> VYSTUP_kontrola_b334.txt
(s ANDROID_JAR a aapt2 v PATH navic prelozi manifest se zdroji a overi "aapt2 dump badging")
"""
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile

ZDE = os.path.dirname(os.path.abspath(__file__))
R = os.path.normpath(os.path.join(ZDE, '../..'))


def cti(p):
    return open(os.path.join(R, p), encoding='utf-8').read()


GR = cti('app/build.gradle')
CM = cti('app/src/main/cpp/CMakeLists.txt')
MF = cti('app/src/main/AndroidManifest.xml')
MA = cti('app/src/main/java/eu/atarihelp/emu10/MainActivity.java')
ND = cti('app/src/main/java/eu/atarihelp/emu10/NapDalkovy.java')
NO = cti('app/src/main/java/eu/atarihelp/emu10/NapOvladac.java')
JS = cti('app/src/main/assets/spolecne/nap_tv_ovladani.js')
NCZ = cti('app/src/main/assets/napoveda/index.html')
NEN = cti('app/src/main/assets/napoveda/en.html')
CI = cti('.github/workflows/build.yml')
vystup, chyb = [], 0


def over(ok, co):
    global chyb
    if not ok:
        chyb += 1
    vystup.append(('OK     ' if ok else 'CHYBA  ') + co)


# ---- A) architektury ----
over("abiFilters 'arm64-v8a', 'armeabi-v7a'" in GR, "A1 build.gradle: abiFilters 'arm64-v8a', 'armeabi-v7a' (TV boxy s 32bitovym Androidem)")
arm = CM[CM.index('if(ANDROID_ABI STREQUAL "armeabi-v7a")', CM.index('set(PS1_DIR')):]
arm = arm[:arm.index('set(NAP_PS1_GPU_DEFS')]
asm = ['gte_arm.S', 'gte_nf_arm.S', 'gte_neon.S', 'gte_nf_neon.S', 'linkage_arm.S', 'arm_utils.S', 'psx_gpu_arm_neon.S']
over(all(a in arm for a in asm), 'A2 PS1 pro armeabi-v7a: dynarec ARM + GTE ARM / NEON + gpu_neon NEON + zvuk (%s)' % ', '.join(asm))
over('GPU_NEON NEON_BUILD ${NAP_PS1_GPU_DEFS} TEXTURE_CACHE_4BPP' in CM and 'set(NAP_PS1_GPU_DEFS SIMD_BUILD)' in CM
     and '${PS1_DIR}/plugins/gpu_neon/psx_gpu/psx_gpu_simd.c)' in CM and '${NAP_PS1_ARCH_SRC}' in CM,
     'A3 ARM64 beze zmeny (SIMD_BUILD + psx_gpu_simd.c + gte_arm64 / linkage_arm64), armeabi-v7a bez SIMD_BUILD (assembler)')
over('target_compile_options(napps1core PRIVATE -marm -mfpu=neon)' in CM, 'A4 armeabi-v7a: -marm -mfpu=neon (jako LOCAL_ARM_MODE arm + LOCAL_ARM_NEON v jni/Android.mk jadra)')
over(CM.count('NOT (ANDROID_ABI STREQUAL "arm64-v8a" OR ANDROID_ABI STREQUAL "armeabi-v7a")') == 2,
     'A5 PS1 i Atari se stavi pro arm64-v8a i armeabi-v7a (jine ABI dal preskocene)')

# ---- T) Android TV ----
over('<uses-feature android:name="android.software.leanback" android:required="false" />' in MF
     and '<uses-feature android:name="android.hardware.touchscreen" android:required="false" />' in MF,
     'T1 manifest: leanback (TV) a dotykovy displej nejsou podminkou instalace')
over('<category android:name="android.intent.category.LEANBACK_LAUNCHER" />' in MF and 'android:banner="@drawable/nap_tv_banner"' in MF,
     'T2 manifest: ikona v nabidce Android TV (LEANBACK_LAUNCHER + banner)')
ban = os.path.join(R, 'app/src/main/res/drawable-xhdpi/nap_tv_banner.png')
wh = (0, 0)
if os.path.exists(ban):
    d = open(ban, 'rb').read(24)
    if d[:8] == b'\x89PNG\r\n\x1a\n':
        wh = struct.unpack('>II', d[16:24])
over(wh == (320, 180), 'T3 banner drawable-xhdpi/nap_tv_banner.png %dx%d (Android TV chce 320 x 180)' % wh)
aj = sys.argv[1] if len(sys.argv) > 1 else ''
if aj and shutil.which('aapt2'):
    t = tempfile.mkdtemp()
    try:
        subprocess.run(['aapt2', 'compile', '--dir', os.path.join(R, 'app/src/main/res'), '-o', t + '/res.zip'], check=True, capture_output=True)
        m = MF.replace('<manifest ', '<manifest package="eu.atarihelp.emu10.test" android:versionCode="377" android:versionName="x" ', 1)
        open(t + '/AndroidManifest.xml', 'w', encoding='utf-8').write(m)
        subprocess.run(['aapt2', 'link', '-o', t + '/t.apk', '-I', aj, '--manifest', t + '/AndroidManifest.xml', '--min-sdk-version', '24',
                        '--target-sdk-version', '34', t + '/res.zip'], check=True, capture_output=True)
        bd = subprocess.run(['aapt2', 'dump', 'badging', t + '/t.apk'], capture_output=True, text=True).stdout
        over("leanback-launchable-activity: name='eu.atarihelp.emu10.MainActivity'" in bd and "banner='res/drawable-xhdpi" in bd
             and "uses-feature-not-required: name='android.hardware.touchscreen'" in bd,
             'T4 aapt2 dump badging: leanback-launchable-activity MainActivity, banner, touchscreen not-required')
    except Exception as e:
        over(False, 'T4 aapt2: %s' % e)
    finally:
        shutil.rmtree(t, ignore_errors=True)
else:
    vystup.append('(T4 aapt2 preskoceno - bez ANDROID_JAR)')

# ---- D) dalkovy ovladac ----
dk = MA[MA.index('public boolean dispatchKeyEvent(KeyEvent e) {'):]
dk = dk[:dk.index('return super.dispatchKeyEvent(e);')]
over(dk.index('ovladac.zOvladace(e) && ovladacKlavesa(e)') < dk.index('napDalkovyKlavesa(e)'),
     'D1 dispatchKeyEvent: herni ovladac (B320) prvni, pak dalkovy ovladac TV (B334), pak WebView')
nk = MA[MA.index('private boolean napDalkovyKlavesa(KeyEvent e) {'):]
nk = nk[:nk.index('/** Stranka appky nactena')]
over('NapDalkovy.preskociDemo(kod)' in nk and 'demoKonec(' in nk and 'NapDalkovy.proMenu(kod)' in nk and 'ovladacMenu(k, dolu' in nk
     and 'NapDalkovy.proHru(kod)' in nk and 'NativePs1CoreBridge.ovladacKlavesaSafe(k, dolu)' in nk
     and 'NativeSegaCoreBridge.ovladacKlavesaSafe(k, dolu)' in nk and 'NativeAtariCoreBridge.devOvladacKlavesaSafe(k, dolu)' in nk
     and 'web.requestFocus()' in nk,
     'D2 dalkovy ovladac: demo (preskocit), hlavni menu (jako gamepad), PS1 / Sega / Atari (jako gamepad), stranky (focus do WebView)')
over('KEYBOARD_TYPE_ALPHABETIC' in ND and 'SOURCE_DPAD' in ND, 'D3 klavesnice s pismeny zustava jako drive (B308 / B320), dalkovy = zdroj DPAD / klavesnice bez pismen')
over('napTvStrankaNactena(url);' in MA and 'NapTvOvladani.zapni()' in MA and 'napTvVlozOvladani()' in MA,
     'D4 stranka nactena na TV: focus do WebView, ramecek hned (zapni), stranka bez skriptu ho dostane dodatecne')
over('sp.getInt("rezim", NapDalkovy.jeTv(c) ? REZIM_SKRYT : REZIM_AUTO)' in NO, 'D5 na TV dotykove ovladani ve hrach vychozi SKRYT (dokud se nezmeni v OPTIONS)')
over('"B334 TV: "' in MA and 'nativeLibraryDir' in MA, 'D6 log: Android TV ano / ne, ABI procesoru, knihovny appky (32 / 64 bitu), Android, model')

# ---- S) stranky ----
r = subprocess.run([sys.executable, os.path.join(ZDE, 'vloz_tv_ovladani.py'), '--kontrola'], capture_output=True, text=True)
over(r.returncode == 0, 'S1 ' + r.stdout.strip().splitlines()[-1] + ' - skript ovladani je na 16 strankach PRVNI')
try:
    js = ("const a=require('/opt/npm-tools/node_modules/acorn');const fs=require('fs');"
          "a.parse(fs.readFileSync(process.argv[1],'utf8'),{ecmaVersion:5});console.log('ok');")
    r = subprocess.run(['node', '-e', js, os.path.join(R, 'app/src/main/assets/spolecne/nap_tv_ovladani.js')], capture_output=True, text=True, timeout=60)
    over(r.stdout.strip() == 'ok', 'S2 nap_tv_ovladani.js je ES5 (starsi WebView v televizich)' + ('' if r.stdout.strip() == 'ok' else ': ' + r.stderr[:200]))
except Exception as e:
    over(False, 'S2 ES5: %s' % e)
over("W.addEventListener('keydown', klavesa, false)" in JS and 'e.defaultPrevented' in JS,
     'S3 skript posloucha az po strance (karusel, volba jazyka si sipky resi samy)')

# ---- N) napoveda, verze, CI ----
over('Appka přímo v televizi (Android TV)' in NCZ and 'The app right on the TV (Android TV)' in NEN, 'N1 napoveda CZ / EN: Android TV (instalace, ovladac TV, gamepad)')
over('versionCode 377' in GR and 'versionName "EMU10-B334-ANDROID-TV"' in GR, 'N2 build.gradle: versionCode 377 "EMU10-B334-ANDROID-TV"')
over("^native-code|^leanback-launchable-activity" in CI and "startswith('lib/')" in CI, 'N3 CI: v logu buildu architektury knihoven APK a nabidka TV')
for s in ('test_overeni/b332/stavba_jadra_gpu.sh', 'test_overeni/b320/stavba_vykreslovace.sh'):
    over("vys.append('plugins/gpu_neon/psx_gpu/psx_gpu_simd.c')" in cti(s), 'N4 %s: psx_gpu_simd.c i po presunu do vetve ARM64' % s)

vystup.append('VYSLEDEK: %d kontrol, %d CHYBA' % (sum(1 for v in vystup if v.startswith(('OK', 'CHYBA'))), chyb))
print('\n'.join(vystup))
open(os.path.join(ZDE, 'VYSTUP_kontrola_b334.txt'), 'w', encoding='utf-8').write('\n'.join(vystup) + '\n')
sys.exit(1 if chyb else 0)
