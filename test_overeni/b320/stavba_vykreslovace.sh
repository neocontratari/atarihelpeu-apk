#!/bin/bash
# B320: SKUTECNY VYKRESLOVAC PS1 z appky pro PC - gpulib + gpu_neon (vendor/pcsx_rearmed, stejne
# soubory jako CMakeLists.txt v bloku "PROVERENY VYKRESLOVAC gpu_neon"). gpu_neon ma variantu
# SIMD_BUILD (psx_gpu_simd.c) - na ARM64 v telefonu NEON, tady na x86_64 SSE (tentyz zdrojak, stejne
# vysledky bod po bodu, vcetne ditheringu a 2x Enhanced Resolution).
# Pouziti: bash stavba_vykreslovace.sh PRAC   -> PRAC/vykreslovac/*.o
# (jadro bez vykreslovace: ../b318/stavba_jadra.sh PRAC -> PRAC/jadro/*.o)
set -euo pipefail
ZDE=$(cd "$(dirname "$0")" && pwd)
KOREN=$(cd "$ZDE/../.." && pwd)
P=$KOREN/app/src/main/cpp/vendor/pcsx_rearmed
PRAC=${1:?PRAC}
OUT=$PRAC/vykreslovac
mkdir -p "$OUT"
# definice a cesty jako ../b318/stavba_jadra.sh (= target_compile_definitions napps1core + DRC_DISABLE)
DEFS="-DGPU_NEON -DNEON_BUILD -DSIMD_BUILD -DTEXTURE_CACHE_4BPP -DTEXTURE_CACHE_8BPP -DHAVE_CHD -DZ7_ST
 -DZSTD_DISABLE_ASM -DHAVE_LIBRETRO -DNO_FRONTEND -DREARMED -DP_HAVE_MMAP=1 -DP_HAVE_PTHREAD=1
 -DP_HAVE_POSIX_MEMALIGN=1 -DUSE_LIBRETRO_VFS -DLIGHTREC_CUSTOM_MAP=0
 -DLIGHTREC_ENABLE_THREADED_COMPILER=0 -DLIGHTREC_ENABLE_DISASSEMBLER=0 -DLIGHTREC_NO_DEBUG=1
 -DUSE_ASYNC_CDROM -DUSE_ASYNC_GPU -DUSE_ASYNC_SPU -DHAVE_RTHREADS -DNDEBUG -DDRC_DISABLE"
INCS="-I$P/include -I$P -I$P/libpcsxcore -I$P/frontend -I$P/deps/crypto -I$P/deps/libchdr/include
 -I$P/deps/libchdr/deps/lzma-24.05/include -I$P/deps/libchdr/deps/zstd-1.5.6/lib
 -I$P/deps/libretro-common/include -I$P/plugins/gpu_neon -I$P/plugins/gpu_neon/psx_gpu"
CFL="-O2 -g -fPIC -ffast-math -fno-strict-aliasing -msse4.2 -w"
DEFS=$(echo $DEFS); INCS=$(echo $INCS)
# zdroje vykreslovace z CMakeLists.txt (add_library napps1core: plugins/gpulib/* a plugins/gpu_neon/*)
python3 - "$KOREN/app/src/main/cpp/CMakeLists.txt" > "$OUT/seznam.txt" <<'PY'
import re, sys
t = open(sys.argv[1], encoding='utf-8').read()
blok = t[t.index('add_library(napps1core SHARED'):]
blok = blok[:blok.index('\n)')]
vys = []
for m in re.finditer(r'\$\{PS1_DIR\}/(\S+)', blok):
    s = m.group(1)
    if (s.startswith('plugins/gpu_neon/') or s.startswith('plugins/gpulib/')) and not s.endswith('.S') and s not in vys:
        vys.append(s)
# B334: psx_gpu_simd.c je od B334 ve vetvi ARM64 seznamu NAP_PS1_ARCH_SRC (armeabi-v7a ma assembler NEON)
if 'plugins/gpu_neon/psx_gpu/psx_gpu_simd.c' not in vys:
    vys.append('plugins/gpu_neon/psx_gpu/psx_gpu_simd.c')
print('\n'.join(vys))
PY
for s in $(cat "$OUT/seznam.txt"); do
  gcc -std=gnu11 $CFL $DEFS $INCS -c "$P/$s" -o "$OUT/$(echo "$s" | tr '/' '_').o" || { echo "CHYBA PREKLADU: $s" >&2; exit 1; }
done
echo "vykreslovac: $(wc -l < "$OUT/seznam.txt") souboru -> $OUT ($(cat "$OUT/seznam.txt" | tr '\n' ' '))"
