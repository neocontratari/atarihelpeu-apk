#!/bin/bash
# B334: SKUTECNE jadro PS1 z appky pro 32bitovy ARM (armeabi-v7a - Android TV) prelozene krizove pro
# Linux armhf (arm-linux-gnueabihf-gcc) a spoustene v qemu-arm: STEJNE soubory a definice jako
# CMakeLists.txt (napps1core) pro ANDROID_ABI armeabi-v7a - dynarec ari64 (assem_arm.c +
# linkage_arm.S), GTE v assembleru ARM / NEON, vykreslovac gpu_neon v assembleru NEON
# (psx_gpu_arm_neon.S), zvuk arm_utils.S, -marm -mfpu=neon.
# Navic se kazdy .S prelozi i clangem pro Android (--target=armv7a-linux-androideabi24) - jako NDK.
# Pouziti: bash stavba_jadra_arm32.sh PRAC        -> PRAC/jadro_arm32/*.o (+ seznam.txt)
set -euo pipefail
ZDE=$(cd "$(dirname "$0")" && pwd)
KOREN=$(cd "$ZDE/../.." && pwd)
P=$KOREN/app/src/main/cpp/vendor/pcsx_rearmed
PRAC=${1:?PRAC}
OUT=$PRAC/jadro_arm32
mkdir -p "$OUT"

# target_compile_definitions(napps1core ...) z CMakeLists.txt pro armeabi-v7a (NAP_PS1_GPU_DEFS prazdne
# = bez SIMD_BUILD); ANDROID ne (Linux armhf - hlavicky Androidu nejsou, jadro je nepotrebuje)
DEFS="-DGPU_NEON -DNEON_BUILD -DTEXTURE_CACHE_4BPP -DTEXTURE_CACHE_8BPP -DHAVE_CHD -DZ7_ST
 -DZSTD_DISABLE_ASM -DHAVE_LIBRETRO -DNO_FRONTEND -DREARMED -DP_HAVE_MMAP=1 -DP_HAVE_PTHREAD=1
 -DP_HAVE_POSIX_MEMALIGN=1 -DUSE_LIBRETRO_VFS -DNDRC_THREAD -DLIGHTREC_CUSTOM_MAP=0
 -DLIGHTREC_ENABLE_THREADED_COMPILER=0 -DLIGHTREC_ENABLE_DISASSEMBLER=0 -DLIGHTREC_NO_DEBUG=1
 -DUSE_ASYNC_CDROM -DUSE_ASYNC_GPU -DUSE_ASYNC_SPU -DHAVE_RTHREADS -DNDEBUG"
INCS="-I$P/include -I$P -I$P/libpcsxcore -I$P/frontend -I$P/deps/crypto -I$P/deps/libchdr/include
 -I$P/deps/libchdr/deps/lzma-24.05/include -I$P/deps/libchdr/deps/zstd-1.5.6/lib
 -I$P/deps/libretro-common/include -I$P/plugins/gpu_neon -I$P/plugins/gpu_neon/psx_gpu"
ARM="-marm -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=hard"
CFL="-O2 -g -fPIC -ffast-math -fno-strict-aliasing -w"

# seznam zdroju z CMakeLists.txt: add_library napps1core (bez nap_ps1_native.cpp - preklada se v testu)
# + vetev armeabi-v7a seznamu NAP_PS1_ARCH_SRC
python3 - "$KOREN/app/src/main/cpp/CMakeLists.txt" > "$OUT/seznam.txt" <<'PY'
import re, sys
t = open(sys.argv[1], encoding='utf-8').read()
blok = t[t.index('add_library(napps1core SHARED'):]
blok = blok[:blok.index('\n)')]
arm = t[t.index('if(ANDROID_ABI STREQUAL "armeabi-v7a")', t.index('set(PS1_DIR')):]
arm = arm[:arm.index('set(NAP_PS1_GPU_DEFS')]
vys = []
for m in list(re.finditer(r'\$\{PS1_DIR\}/(\S+)', blok)) + list(re.finditer(r'\$\{PS1_DIR\}/(\S+?)\)?\s', arm)):
    s = m.group(1).rstrip(')')
    if s not in vys:
        vys.append(s)
print('\n'.join(vys))
PY
DEFS=$(echo $DEFS); INCS=$(echo $INCS)
export P OUT CFL DEFS INCS ARM
prekladej() {
  local s=$1 o
  o="$OUT/$(echo "$s" | tr '/' '_').o"
  case "$s" in
    *.S) arm-linux-gnueabihf-gcc $ARM $CFL $DEFS $INCS -c "$P/$s" -o "$o" || { echo "CHYBA PREKLADU: $s" >&2; return 1; }
         # i clang jako NDK (integrovany assembler, Android armv7a)
         clang --target=armv7a-linux-androideabi24 -marm -mfpu=neon -mfloat-abi=softfp $DEFS -DANDROID $INCS -c "$P/$s" -o "$o.clang" \
           || { echo "CHYBA CLANG (NDK): $s" >&2; return 1; } ;;
    *)   arm-linux-gnueabihf-gcc -std=gnu11 $ARM $CFL $DEFS $INCS -c "$P/$s" -o "$o" || { echo "CHYBA PREKLADU: $s" >&2; return 1; } ;;
  esac
}
export -f prekladej
xargs -P "$(nproc)" -n 1 bash -c 'prekladej "$0"' < "$OUT/seznam.txt"
n=$(wc -l < "$OUT/seznam.txt")
echo "jadro PS1 pro 32bitovy ARM: $n souboru -> $OUT"
grep -E '\.S$|new_dynarec|gpu_neon' "$OUT/seznam.txt" | sed 's/^/  vcetne /'
