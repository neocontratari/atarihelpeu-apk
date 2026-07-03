cmake_minimum_required(VERSION 3.22.1)
project(napsega_native_proof LANGUAGES C CXX)

# BUILD2QB: offline/vendor ClownMDEmu-core build.
# No FetchContent, no GitHub during GitHub Actions, no submodule update,
# no disassembler tools, no microcode generator executable, no tests.
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(NAP_VENDOR_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/clownmdemu-core)
set(NAP_CORE_DIR ${NAP_VENDOR_DIR}/source)
set(NAP_COMMON_DIR ${NAP_VENDOR_DIR}/libraries/clowncommon)
set(NAP_M68K_DIR ${NAP_VENDOR_DIR}/libraries/clown68000)
set(NAP_Z80_DIR ${NAP_VENDOR_DIR}/libraries/clownz80)

# BUILD2RY CRITICAL PERFORMANCE FIX:
# GitHub Actions vyrabi DEBUG APK => CMake Debug => vsechny .c soubory jadra
# (68000/Z80 interpretery, VDP, FM) se stavely s -O0 (BEZ optimalizace).
# Namereno v BUILD2RX auditu: S8 coreAvgMs=17-18ms uz za studena pri max taktu
# (NTSC budget je 16.67ms) -> hudba chrcela od zacatku; po CPU throttlingu
# (2314->1703 MHz behem ~100s) coreAvgMs=24.6ms -> kolaps zvuku.
# Nox (silne PC jadro) utahl i -O0 (8.4ms), proto tam zvuk drzel.
# Tyto flagy vynuti optimalizovane jadro NEZAVISLE na debug/release variante.
set(NAP_CORE_OPT_FLAGS -O3 -fomit-frame-pointer)

if(NOT EXISTS ${NAP_CORE_DIR}/clownmdemu.h)
    message(FATAL_ERROR "BUILD2QB: offline vendor missing source/clownmdemu.h")
endif()
if(NOT EXISTS ${NAP_M68K_DIR}/source/interpreter/clown68000.c)
    message(FATAL_ERROR "BUILD2QB: offline vendor missing clown68000 interpreter")
endif()
if(NOT EXISTS ${NAP_Z80_DIR}/source/interpreter.c)
    message(FATAL_ERROR "BUILD2QB: offline vendor missing clownz80 interpreter")
endif()
if(NOT EXISTS ${NAP_COMMON_DIR}/clowncommon.h)
    message(FATAL_ERROR "BUILD2QB: offline vendor missing clowncommon.h")
endif()

add_library(clown68000-common STATIC
        ${NAP_M68K_DIR}/source/common/opcode.c)
target_compile_definitions(clown68000-common PUBLIC CC_USE_C99_INTEGERS=1)
target_compile_options(clown68000-common PRIVATE ${NAP_CORE_OPT_FLAGS}) # BUILD2RY: -O3 i v debug APK
target_include_directories(clown68000-common PUBLIC
        ${NAP_M68K_DIR}/source/common
        ${NAP_M68K_DIR}/libraries/clowncommon)

add_library(clown68000-interpreter STATIC
        ${NAP_M68K_DIR}/source/interpreter/clown68000.c)
target_compile_definitions(clown68000-interpreter PUBLIC CC_USE_C99_INTEGERS=1)
target_compile_options(clown68000-interpreter PRIVATE ${NAP_CORE_OPT_FLAGS}) # BUILD2RY: -O3 i v debug APK
target_include_directories(clown68000-interpreter PUBLIC
        ${NAP_M68K_DIR}/source/interpreter
        ${NAP_M68K_DIR}/source/common
        ${NAP_M68K_DIR}/libraries/clowncommon)
target_link_libraries(clown68000-interpreter PRIVATE clown68000-common)

add_library(clownz80-common STATIC
        ${NAP_Z80_DIR}/source/common.c)
target_compile_definitions(clownz80-common PUBLIC CC_USE_C99_INTEGERS=1)
target_compile_options(clownz80-common PRIVATE ${NAP_CORE_OPT_FLAGS}) # BUILD2RY: -O3 i v debug APK
target_include_directories(clownz80-common PUBLIC
        ${NAP_Z80_DIR}/source
        ${NAP_Z80_DIR}/libraries/clowncommon)

add_library(clownz80-interpreter STATIC
        ${NAP_Z80_DIR}/source/interpreter.c)
target_compile_definitions(clownz80-interpreter PUBLIC CC_USE_C99_INTEGERS=1)
target_compile_options(clownz80-interpreter PRIVATE ${NAP_CORE_OPT_FLAGS}) # BUILD2RY: -O3 i v debug APK
target_include_directories(clownz80-interpreter PUBLIC
        ${NAP_Z80_DIR}/source
        ${NAP_Z80_DIR}/libraries/clowncommon)
target_link_libraries(clownz80-interpreter PRIVATE clownz80-common)

add_library(clownmdemu-core STATIC
        ${NAP_CORE_DIR}/bus-common.c
        ${NAP_CORE_DIR}/bus-main-m68k.c
        ${NAP_CORE_DIR}/bus-sub-m68k.c
        ${NAP_CORE_DIR}/bus-z80.c
        ${NAP_CORE_DIR}/cdc.c
        ${NAP_CORE_DIR}/cdda.c
        ${NAP_CORE_DIR}/clownmdemu.c
        ${NAP_CORE_DIR}/controller.c
        ${NAP_CORE_DIR}/controller-manager.c
        ${NAP_CORE_DIR}/controller-multitap-ea.c
        ${NAP_CORE_DIR}/controller-multitap-sega.c
        ${NAP_CORE_DIR}/fm.c
        ${NAP_CORE_DIR}/fm-channel.c
        ${NAP_CORE_DIR}/fm-lfo.c
        ${NAP_CORE_DIR}/fm-operator.c
        ${NAP_CORE_DIR}/fm-phase.c
        ${NAP_CORE_DIR}/io-port.c
        ${NAP_CORE_DIR}/log.c
        ${NAP_CORE_DIR}/low-pass-filter.c
        ${NAP_CORE_DIR}/pcm.c
        ${NAP_CORE_DIR}/psg.c
        ${NAP_CORE_DIR}/sync.c
        ${NAP_CORE_DIR}/vdp.c)
target_compile_definitions(clownmdemu-core PUBLIC CC_USE_C99_INTEGERS=1)
target_compile_options(clownmdemu-core PRIVATE ${NAP_CORE_OPT_FLAGS}) # BUILD2RY: -O3 i v debug APK
target_include_directories(clownmdemu-core PUBLIC
        ${NAP_CORE_DIR}
        ${NAP_COMMON_DIR}
        ${NAP_M68K_DIR}/source/interpreter
        ${NAP_Z80_DIR}/source)
target_link_libraries(clownmdemu-core PRIVATE clown68000-interpreter clownz80-interpreter)

add_library(napsega_native_proof SHARED
        nap_sega_native_proof.cpp)

target_compile_features(napsega_native_proof PRIVATE cxx_std_17)
target_compile_options(napsega_native_proof PRIVATE ${NAP_CORE_OPT_FLAGS}) # BUILD2RY: -O3 i v debug APK
target_compile_definitions(napsega_native_proof PRIVATE
        NAP_SEGA_VENDOR_CORE_PRESENT=1
        NAP_SEGA_VENDOR_CLOWNMDEMU_OFFLINE=1
        NAP_SEGA_BUILD2QB=1)

target_include_directories(napsega_native_proof PRIVATE
        ${NAP_CORE_DIR}
        ${NAP_COMMON_DIR}
        ${NAP_M68K_DIR}/source/interpreter
        ${NAP_Z80_DIR}/source)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(napsega_native_proof PRIVATE
        ${android-lib}
        ${log-lib}
        clownmdemu-core)
# ============ BUILD2SA1: PCSX-ReARMed PS1 core (interpreter, bez dynarecu - ten prijde v SA1.5) ============
set(PS1_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/pcsx_rearmed)
add_library(napps1core SHARED
    ${CMAKE_CURRENT_LIST_DIR}/nap_ps1_native.cpp
    ${PS1_DIR}/deps/libchdr/deps/lzma-24.05/src/Alloc.c
    ${PS1_DIR}/deps/libchdr/deps/lzma-24.05/src/CpuArch.c
    ${PS1_DIR}/deps/libchdr/deps/lzma-24.05/src/Delta.c
    ${PS1_DIR}/deps/libchdr/deps/lzma-24.05/src/LzFind.c
    ${PS1_DIR}/deps/libchdr/deps/lzma-24.05/src/LzmaDec.c
    ${PS1_DIR}/deps/libchdr/deps/lzma-24.05/src/LzmaEnc.c
    ${PS1_DIR}/deps/libchdr/deps/lzma-24.05/src/Sort.c
    ${PS1_DIR}/deps/libchdr/deps/zlib-1.3.1/adler32.c
    ${PS1_DIR}/deps/libchdr/deps/zlib-1.3.1/compress.c
    ${PS1_DIR}/deps/libchdr/deps/zlib-1.3.1/crc32.c
    ${PS1_DIR}/deps/libchdr/deps/zlib-1.3.1/deflate.c
    ${PS1_DIR}/deps/libchdr/deps/zlib-1.3.1/gzclose.c
    ${PS1_DIR}/deps/libchdr/deps/zlib-1.3.1/gzlib.c
    ${PS1_DIR}/deps/libchdr/deps/zlib-1.3.1/gzread.c
    ${PS1_DIR}/deps/libchdr/deps/zlib-1.3.1/gzwrite.c
    ${PS1_DIR}/deps/libchdr/deps/zlib-1.3.1/infback.c
    ${PS1_DIR}/deps/libchdr/deps/zlib-1.3.1/inffast.c
    ${PS1_DIR}/deps/libchdr/deps/zlib-1.3.1/inflate.c
    ${PS1_DIR}/deps/libchdr/deps/zlib-1.3.1/inftrees.c
    ${PS1_DIR}/deps/libchdr/deps/zlib-1.3.1/trees.c
    ${PS1_DIR}/deps/libchdr/deps/zlib-1.3.1/uncompr.c
    ${PS1_DIR}/deps/libchdr/deps/zlib-1.3.1/zutil.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/common/entropy_common.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/common/error_private.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/common/fse_decompress.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/common/xxhash.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/common/zstd_common.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/decompress/huf_decompress.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/decompress/zstd_ddict.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/decompress/zstd_decompress.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/decompress/zstd_decompress_block.c
    ${PS1_DIR}/deps/libchdr/src/libchdr_bitstream.c
    ${PS1_DIR}/deps/libchdr/src/libchdr_cdrom.c
    ${PS1_DIR}/deps/libchdr/src/libchdr_chd.c
    ${PS1_DIR}/deps/libchdr/src/libchdr_flac.c
    ${PS1_DIR}/deps/libchdr/src/libchdr_huffman.c
    ${PS1_DIR}/deps/libretro-common/compat/compat_posix_string.c
    ${PS1_DIR}/deps/libretro-common/compat/compat_strl.c
    ${PS1_DIR}/deps/libretro-common/compat/fopen_utf8.c
    ${PS1_DIR}/deps/libretro-common/encodings/encoding_utf.c
    ${PS1_DIR}/deps/libretro-common/features/features_cpu.c
    ${PS1_DIR}/deps/libretro-common/file/file_path.c
    ${PS1_DIR}/deps/libretro-common/lists/string_list.c
    ${PS1_DIR}/deps/libretro-common/memmap/memalign.c
    ${PS1_DIR}/deps/libretro-common/streams/file_stream.c
    ${PS1_DIR}/deps/libretro-common/streams/file_stream_transforms.c
    ${PS1_DIR}/deps/libretro-common/string/stdstring.c
    ${PS1_DIR}/deps/libretro-common/time/rtime.c
    ${PS1_DIR}/deps/libretro-common/vfs/vfs_implementation.c
    ${PS1_DIR}/deps/libretro-common/vfs/vfs_implementation_cdrom.c
    ${PS1_DIR}/frontend/cspace.c
    ${PS1_DIR}/frontend/libretro-cdrom.c
    ${PS1_DIR}/frontend/libretro.c
    ${PS1_DIR}/frontend/main.c
    ${PS1_DIR}/frontend/pcsxr-threads.c
    ${PS1_DIR}/frontend/plugin.c
    ${PS1_DIR}/libpcsxcore/cdriso.c
    ${PS1_DIR}/libpcsxcore/cdrom-async.c
    ${PS1_DIR}/libpcsxcore/cdrom.c
    ${PS1_DIR}/libpcsxcore/cheat.c
    ${PS1_DIR}/libpcsxcore/database.c
    ${PS1_DIR}/libpcsxcore/decode_xa.c
    ${PS1_DIR}/libpcsxcore/gpu.c
    ${PS1_DIR}/libpcsxcore/gte.c
    ${PS1_DIR}/libpcsxcore/gte_divider.c
    ${PS1_DIR}/libpcsxcore/gte_nf.c
    ${PS1_DIR}/libpcsxcore/mdec.c
    ${PS1_DIR}/libpcsxcore/misc.c
    ${PS1_DIR}/libpcsxcore/new_dynarec/emu_if.c
    ${PS1_DIR}/libpcsxcore/pad.c
    ${PS1_DIR}/libpcsxcore/plugins.c
    ${PS1_DIR}/libpcsxcore/ppf.c
    ${PS1_DIR}/libpcsxcore/psxbios.c
    ${PS1_DIR}/libpcsxcore/psxcommon.c
    ${PS1_DIR}/libpcsxcore/psxcounters.c
    ${PS1_DIR}/libpcsxcore/psxdma.c
    ${PS1_DIR}/libpcsxcore/psxevents.c
    ${PS1_DIR}/libpcsxcore/psxhw.c
    ${PS1_DIR}/libpcsxcore/psxinterpreter.c
    ${PS1_DIR}/libpcsxcore/psxmem.c
    ${PS1_DIR}/libpcsxcore/r3000a.c
    ${PS1_DIR}/libpcsxcore/sio.c
    ${PS1_DIR}/libpcsxcore/spu.c
    ${PS1_DIR}/plugins/dfsound/dma.c
    ${PS1_DIR}/plugins/dfsound/freeze.c
    ${PS1_DIR}/plugins/dfsound/nullsnd.c
    ${PS1_DIR}/plugins/dfsound/out.c
    ${PS1_DIR}/plugins/dfsound/registers.c
    ${PS1_DIR}/plugins/dfsound/spu.c
    ${PS1_DIR}/plugins/gpu_neon/psx_gpu/psx_gpu_simd.c
    ${PS1_DIR}/plugins/gpu_neon/psx_gpu_if.c
    ${PS1_DIR}/plugins/gpulib/gpu.c
    ${PS1_DIR}/plugins/gpulib/gpu_async.c
    ${PS1_DIR}/plugins/gpulib/prim.c
    ${PS1_DIR}/plugins/gpulib/vout_pl.c)
target_include_directories(napps1core PRIVATE
    ${PS1_DIR}/include ${PS1_DIR} ${PS1_DIR}/libpcsxcore ${PS1_DIR}/frontend
    ${PS1_DIR}/deps/libchdr/include ${PS1_DIR}/deps/libchdr/deps/zlib-1.3.1
    ${PS1_DIR}/deps/libchdr/deps/lzma-24.05/include ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib
    ${PS1_DIR}/deps/libretro-common/include)
target_compile_definitions(napps1core PRIVATE
    DRC_DISABLE DISABLE_MEM_LUTS=0 GPU_NEON BUILTIN_GPU=neon NEON_BUILD SIMD_BUILD
    TEXTURE_CACHE_4BPP TEXTURE_CACHE_8BPP HAVE_CHD HAVE_CDROM USE_ASYNC_CDROM USE_ASYNC_GPU USE_ASYNC_SPU
    HAVE_LIBRETRO USE_LIBRETRO_VFS NO_FRONTEND HAVE_RTHREADS HAVE_UNISTD_H NDEBUG Z7_ST ZSTD_DISABLE_ASM=1
    P_HAVE_MMAP=1 P_HAVE_POSIX_MEMALIGN=1)
target_compile_options(napps1core PRIVATE ${NAP_CORE_OPT_FLAGS} -ffast-math -Wno-unused -Wno-unused-function)
target_link_libraries(napps1core ${log-lib} m)
