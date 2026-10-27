#!/bin/bash
# B317: SKUTECNE jadro PS1 z appky (vendor/pcsx_rearmed - stejne soubory jako CMakeLists.txt,
# knihovna napps1core) prelozene pro PC: x86_64, interpret procesoru misto dynarecu
# (DRC_DISABLE - dynarec appky je jen pro ARM), bez vykreslovace gpu_neon (ARM NEON) -
# grafika se v testu mysi nekresli (nahradu dava integrace_b317.cpp).
# Pouziti: bash stavba_jadra.sh PRAC        -> PRAC/jadro/*.o (+ PRAC/jadro/seznam.txt)
#          bash stavba_jadra.sh PRAC GIT    -> navic PRAC/jadro_b316/frontend_plugin.c.o:
#                                              frontend/plugin.c z commitu GIT (B316, bez
#                                              zmeny "posun mysi se precte jednou") pro srovnani
set -euo pipefail
ZDE=$(cd "$(dirname "$0")" && pwd)
KOREN=$(cd "$ZDE/../.." && pwd)
P=$KOREN/app/src/main/cpp/vendor/pcsx_rearmed
PRAC=${1:?PRAC}
OUT=$PRAC/jadro
mkdir -p "$OUT"

# stejne definice jako target_compile_definitions(napps1core ...) v CMakeLists.txt + DRC_DISABLE
# (bez NDRC_THREAD - to je vlakno dynarecu, ktery tu neni)
DEFS="-DGPU_NEON -DNEON_BUILD -DSIMD_BUILD -DTEXTURE_CACHE_4BPP -DTEXTURE_CACHE_8BPP -DHAVE_CHD -DZ7_ST
 -DZSTD_DISABLE_ASM -DHAVE_LIBRETRO -DNO_FRONTEND -DREARMED -DP_HAVE_MMAP=1 -DP_HAVE_PTHREAD=1
 -DP_HAVE_POSIX_MEMALIGN=1 -DUSE_LIBRETRO_VFS -DLIGHTREC_CUSTOM_MAP=0
 -DLIGHTREC_ENABLE_THREADED_COMPILER=0 -DLIGHTREC_ENABLE_DISASSEMBLER=0 -DLIGHTREC_NO_DEBUG=1
 -DUSE_ASYNC_CDROM -DUSE_ASYNC_GPU -DUSE_ASYNC_SPU -DHAVE_RTHREADS -DNDEBUG -DDRC_DISABLE"
INCS="-I$P/include -I$P -I$P/libpcsxcore -I$P/frontend -I$P/deps/crypto -I$P/deps/libchdr/include
 -I$P/deps/libchdr/deps/lzma-24.05/include -I$P/deps/libchdr/deps/zstd-1.5.6/lib
 -I$P/deps/libretro-common/include -I$P/plugins/gpu_neon -I$P/plugins/gpu_neon/psx_gpu"
CFL="-O2 -g -fPIC -ffast-math -fno-strict-aliasing -msse4.2 -w"

# seznam zdroju z CMakeLists.txt (add_library napps1core), bez: nap_ps1_native.cpp (preklada
# se v testu), assembleru a dynarecu pro ARM, vykreslovace (gpu_neon + gpulib - cip GPU nahrazuje
# integrace_b317.cpp)
python3 - "$KOREN/app/src/main/cpp/CMakeLists.txt" > "$OUT/seznam.txt" <<'EOF'
import re, sys
t = open(sys.argv[1], encoding='utf-8').read()
blok = t[t.index('add_library(napps1core SHARED'):]
blok = blok[:blok.index('\n)')]
vys = []
for m in re.finditer(r'\$\{PS1_DIR\}/(\S+)', blok):
    s = m.group(1)
    # dynarec jen pro ARM; z nej jen emu_if.c (s DRC_DISABLE = prazdne nahrady pro interpret)
    if s.endswith('.S') or ('/new_dynarec/' in s and not s.endswith('/emu_if.c')) \
       or s.startswith('plugins/gpu_neon/') or s.startswith('plugins/gpulib/'):
        continue
    if s not in vys:
        vys.append(s)
print('\n'.join(vys))
EOF
DEFS=$(echo $DEFS); INCS=$(echo $INCS)   # na jeden radek
export P OUT CFL DEFS INCS
prekladej() {
  local s=$1 o
  o="$OUT/$(echo "$s" | tr '/' '_').o"
  gcc -std=gnu11 $CFL $DEFS $INCS -c "$P/$s" -o "$o" || { echo "CHYBA PREKLADU: $s" >&2; return 1; }
}
export -f prekladej
xargs -P "$(nproc)" -n 1 bash -c 'prekladej "$0"' < "$OUT/seznam.txt"
n=$(wc -l < "$OUT/seznam.txt")
echo "jadro: $n souboru -> $OUT"

if [ $# -ge 2 ]; then
  OUT0=$PRAC/jadro_b316
  mkdir -p "$OUT0"
  git -C "$KOREN" show "$2:app/src/main/cpp/vendor/pcsx_rearmed/frontend/plugin.c" > "$OUT0/plugin_b316.c"
  gcc -std=gnu11 $CFL $DEFS $INCS -I"$P/frontend" -c "$OUT0/plugin_b316.c" -o "$OUT0/frontend_plugin.c.o"
  echo "frontend/plugin.c z $2 -> $OUT0/frontend_plugin.c.o"
fi
