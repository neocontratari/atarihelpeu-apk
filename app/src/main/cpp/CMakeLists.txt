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
# ============ BUILD2SA1-FIX: PCSX-ReARMed presne dle jni/Android.mk (arm64, interpreter) ============
# KILL-SWITCH: kdyz by PS1 blok zlobil, staci OFF a workflow je zeleny bez PS1.
option(NAP_ENABLE_PS1 "Build PS1 core (napps1core)" ON)
enable_language(ASM) # BUILD2SA6: dynarec ma arm64 assembly (.S)
# BUILD2SA1-FIX3: Gradle stavi APK pro VSECHNY architektury (arm64, armeabi-v7a,
# x86, x86_64). PS1 konfigurace je zamerena na arm64 (S8 a moderni telefony) -
# na ostatnich ABI se PS1 PRESKOCI a Sega se stavi jak dosud. Zadna dalsi
# architektura uz nemuze shodit workflow; Java most na ne-arm64 hlasi
# PS1_CORE_LOAD_FAIL misto padu.
if(ANDROID AND NOT (ANDROID_ABI STREQUAL "arm64-v8a"))
    set(NAP_ENABLE_PS1 OFF)
    message(STATUS "BUILD2SA1: PS1 core skipped for ABI '${ANDROID_ABI}' (arm64-v8a only for now)")
endif()
if(NAP_ENABLE_PS1)
set(PS1_DIR ${CMAKE_CURRENT_LIST_DIR}/vendor/pcsx_rearmed)
add_library(napps1core SHARED
    ${CMAKE_CURRENT_LIST_DIR}/nap_ps1_native.cpp
    ${PS1_DIR}/libpcsxcore/cdriso.c
    ${PS1_DIR}/libpcsxcore/cdrom.c
    ${PS1_DIR}/libpcsxcore/cdrom-async.c
    ${PS1_DIR}/libpcsxcore/cheat.c
    ${PS1_DIR}/libpcsxcore/database.c
    ${PS1_DIR}/libpcsxcore/decode_xa.c
    ${PS1_DIR}/libpcsxcore/mdec.c
    ${PS1_DIR}/libpcsxcore/misc.c
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
    ${PS1_DIR}/libpcsxcore/gpu.c
    ${PS1_DIR}/libpcsxcore/gte.c
    ${PS1_DIR}/libpcsxcore/gte_nf.c
    ${PS1_DIR}/libpcsxcore/gte_divider.c
    ${PS1_DIR}/plugins/dfsound/dma.c
    ${PS1_DIR}/plugins/dfsound/freeze.c
    ${PS1_DIR}/plugins/dfsound/registers.c
    ${PS1_DIR}/plugins/dfsound/spu.c
    ${PS1_DIR}/plugins/dfsound/out.c
    ${PS1_DIR}/plugins/dfsound/nullsnd.c
    ${PS1_DIR}/plugins/gpulib/prim.c
    ${PS1_DIR}/plugins/gpulib/gpu_async.c
    ${PS1_DIR}/plugins/cdrcimg/cdrcimg.c
    ${PS1_DIR}/frontend/main.c
    ${PS1_DIR}/frontend/plugin.c
    ${PS1_DIR}/frontend/cspace.c
    ${PS1_DIR}/frontend/libretro.c
    ${PS1_DIR}/deps/libchdr/src/libchdr_bitstream.c
    ${PS1_DIR}/deps/libchdr/src/libchdr_cdrom.c
    ${PS1_DIR}/deps/libchdr/src/libchdr_chd.c
    ${PS1_DIR}/deps/libchdr/src/libchdr_flac.c
    ${PS1_DIR}/deps/libchdr/src/libchdr_huffman.c
    ${PS1_DIR}/deps/libchdr/deps/lzma-24.05/src/Alloc.c
    ${PS1_DIR}/deps/libchdr/deps/lzma-24.05/src/CpuArch.c
    ${PS1_DIR}/deps/libchdr/deps/lzma-24.05/src/Delta.c
    ${PS1_DIR}/deps/libchdr/deps/lzma-24.05/src/LzFind.c
    ${PS1_DIR}/deps/libchdr/deps/lzma-24.05/src/LzmaDec.c
    ${PS1_DIR}/deps/libchdr/deps/lzma-24.05/src/LzmaEnc.c
    ${PS1_DIR}/deps/libchdr/deps/lzma-24.05/src/Sort.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/common/entropy_common.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/common/error_private.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/common/fse_decompress.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/common/xxhash.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/common/zstd_common.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/decompress/huf_decompress.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/decompress/zstd_ddict.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/decompress/zstd_decompress_block.c
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib/decompress/zstd_decompress.c
    ${PS1_DIR}/deps/libretro-common/compat/compat_posix_string.c
    ${PS1_DIR}/deps/libretro-common/compat/fopen_utf8.c
    ${PS1_DIR}/deps/libretro-common/compat/compat_strl.c
    ${PS1_DIR}/deps/libretro-common/encodings/encoding_utf.c
    ${PS1_DIR}/deps/libretro-common/file/file_path.c
    ${PS1_DIR}/deps/libretro-common/streams/file_stream.c
    ${PS1_DIR}/deps/libretro-common/streams/file_stream_transforms.c
    ${PS1_DIR}/deps/libretro-common/string/stdstring.c
    ${PS1_DIR}/deps/libretro-common/time/rtime.c
    ${PS1_DIR}/deps/libretro-common/vfs/vfs_implementation.c
    ${PS1_DIR}/libpcsxcore/new_dynarec/emu_if.c
    ${PS1_DIR}/libpcsxcore/new_dynarec/new_dynarec.c
    ${PS1_DIR}/libpcsxcore/new_dynarec/pcsxmem.c
    ${PS1_DIR}/libpcsxcore/gte_arm64.S
    ${PS1_DIR}/libpcsxcore/gte_nf_arm64.S
    ${PS1_DIR}/libpcsxcore/new_dynarec/linkage_arm64.S
    ${PS1_DIR}/plugins/gpu-gles/gpulib_if.c
    ${PS1_DIR}/plugins/gpu-gles/gpuFps.c
    ${PS1_DIR}/frontend/pcsxr-threads.c
    ${PS1_DIR}/deps/libretro-common/features/features_cpu.c
)
target_include_directories(napps1core PRIVATE
    ${PS1_DIR}/include ${PS1_DIR} ${PS1_DIR}/libpcsxcore ${PS1_DIR}/frontend
    ${PS1_DIR}/deps/crypto
    ${PS1_DIR}/deps/libchdr/include ${PS1_DIR}/deps/libchdr/deps/lzma-24.05/include
    ${PS1_DIR}/deps/libchdr/deps/zstd-1.5.6/lib ${PS1_DIR}/deps/libretro-common/include
    ${PS1_DIR}/plugins/gpu-gles)
target_compile_definitions(napps1core PRIVATE
    HAVE_CHD Z7_ST ZSTD_DISABLE_ASM HAVE_LIBRETRO NO_FRONTEND REARMED
    P_HAVE_MMAP=1 P_HAVE_PTHREAD=1 P_HAVE_POSIX_MEMALIGN=1 USE_LIBRETRO_VFS
    NDRC_THREAD LIGHTREC_CUSTOM_MAP=0 LIGHTREC_ENABLE_THREADED_COMPILER=0
    LIGHTREC_ENABLE_DISASSEMBLER=0 LIGHTREC_NO_DEBUG=1
    USE_ASYNC_CDROM USE_ASYNC_GPU USE_ASYNC_SPU HAVE_RTHREADS NDEBUG)
    # BUILD2SK98: GPU_NEON/NEON_BUILD/SIMD_BUILD/TEXTURE_CACHE_* zamerne
    # ODEBRANY - patrily gpu_neon (uz nekompilovanem). GPU_NEON v sdilenem
    # prim.c jen menil jeden interni presnostni prepinac (#define STRICT),
    # nic vic - bezpecne vypnout.
if(ANDROID)
    target_compile_definitions(napps1core PRIVATE ANDROID)
endif()
target_compile_options(napps1core PRIVATE ${NAP_CORE_OPT_FLAGS} -ffast-math -fno-strict-aliasing -Wno-unused -Wno-unused-function)
find_package(Threads REQUIRED)
# BUILD2SA1-FIX2: Android.mk ma LOCAL_LDLIBS := -lz -llog. Bez systemove zlib
# (libchdr ji potrebuje) spadl NDK link - presne chyba z Actions #853.
# --no-undefined = stejna prisnost jako NDK, chybejici symbol shodi uz muj test.
# BUILD2SK98: + EGL/GLESv1_CM - standardni, VZDY pritomne Android system
# knihovny (soucast kazdeho Android NDK) - potrebne pro gpu-gles.
target_link_libraries(napps1core ${log-lib} Threads::Threads m z ${CMAKE_DL_LIBS} EGL GLESv1_CM)
target_link_options(napps1core PRIVATE -Wl,--no-undefined -Wl,-z,max-page-size=16384)
endif() # NAP_ENABLE_PS1
