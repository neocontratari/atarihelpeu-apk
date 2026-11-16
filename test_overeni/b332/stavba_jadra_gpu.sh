#!/bin/bash
# B332: SKUTECNE jadro PS1 z appky VCETNE VYKRESLOVACE (gpulib + gpu_neon + kresleni ve vlakne
# gpu_async) prelozene pro PC: x86_64, interpret procesoru misto dynarecu (DRC_DISABLE - dynarec
# appky je jen pro ARM). gpu_neon na PC bezi pres SSE (psx_gpu_simd.c umi ARM NEON i SSE2) -
# stejny kod vykreslovace jako v telefonu, jen jine instrukce procesoru.
# Oproti ../b317/stavba_jadra.sh: vykreslovac se NEvynechava (test kresleni ve vlakne potrebuje
# skutecny gpulib a gpu_neon) a soubory jsou ze stejneho seznamu CMakeLists.txt (napps1core).
# Pouziti: bash stavba_jadra_gpu.sh PRAC        -> PRAC/jadro_gpu/*.o (+ seznam.txt)
set -euo pipefail
ZDE=$(cd "$(dirname "$0")" && pwd)
KOREN=$(cd "$ZDE/../.." && pwd)
P=$KOREN/app/src/main/cpp/vendor/pcsx_rearmed
PRAC=${1:?PRAC}
OUT=$PRAC/jadro_gpu
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

# seznam zdroju z CMakeLists.txt (add_library napps1core), bez: nap_ps1_native.cpp (preklada se
# v testu), assembleru a dynarecu pro ARM. Vykreslovac (gpulib + gpu_neon) ZUSTAVA.
python3 - "$KOREN/app/src/main/cpp/CMakeLists.txt" > "$OUT/seznam.txt" <<'EOF'
import re, sys
t = open(sys.argv[1], encoding='utf-8').read()
blok = t[t.index('add_library(napps1core SHARED'):]
blok = blok[:blok.index('\n)')]
vys = []
for m in re.finditer(r'\$\{PS1_DIR\}/(\S+)', blok):
    s = m.group(1)
    if s.endswith('.S') or ('/new_dynarec/' in s and not s.endswith('/emu_if.c')):
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
echo "jadro s vykreslovacem: $n souboru -> $OUT"
grep -E 'gpulib|gpu_neon|pcsxr-threads' "$OUT/seznam.txt" | sed 's/^/  vcetne /'
