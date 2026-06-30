cmake_minimum_required(VERSION 3.22.1)
project(napsega_native_proof LANGUAGES C CXX)

# BUILD2QG: offline/vendor ClownMDEmu-core build.
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

if(NOT EXISTS ${NAP_CORE_DIR}/clownmdemu.h)
    message(FATAL_ERROR "BUILD2QG: offline vendor missing source/clownmdemu.h")
endif()
if(NOT EXISTS ${NAP_M68K_DIR}/source/interpreter/clown68000.c)
    message(FATAL_ERROR "BUILD2QG: offline vendor missing clown68000 interpreter")
endif()
if(NOT EXISTS ${NAP_Z80_DIR}/source/interpreter.c)
    message(FATAL_ERROR "BUILD2QG: offline vendor missing clownz80 interpreter")
endif()
if(NOT EXISTS ${NAP_COMMON_DIR}/clowncommon.h)
    message(FATAL_ERROR "BUILD2QG: offline vendor missing clowncommon.h")
endif()

add_library(clown68000-common STATIC
        ${NAP_M68K_DIR}/source/common/opcode.c)
target_compile_definitions(clown68000-common PUBLIC CC_USE_C99_INTEGERS=1)
target_include_directories(clown68000-common PUBLIC
        ${NAP_M68K_DIR}/source/common
        ${NAP_M68K_DIR}/libraries/clowncommon)

add_library(clown68000-interpreter STATIC
        ${NAP_M68K_DIR}/source/interpreter/clown68000.c)
target_compile_definitions(clown68000-interpreter PUBLIC CC_USE_C99_INTEGERS=1)
target_include_directories(clown68000-interpreter PUBLIC
        ${NAP_M68K_DIR}/source/interpreter
        ${NAP_M68K_DIR}/source/common
        ${NAP_M68K_DIR}/libraries/clowncommon)
target_link_libraries(clown68000-interpreter PRIVATE clown68000-common)

add_library(clownz80-common STATIC
        ${NAP_Z80_DIR}/source/common.c)
target_compile_definitions(clownz80-common PUBLIC CC_USE_C99_INTEGERS=1)
target_include_directories(clownz80-common PUBLIC
        ${NAP_Z80_DIR}/source
        ${NAP_Z80_DIR}/libraries/clowncommon)

add_library(clownz80-interpreter STATIC
        ${NAP_Z80_DIR}/source/interpreter.c)
target_compile_definitions(clownz80-interpreter PUBLIC CC_USE_C99_INTEGERS=1)
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
target_include_directories(clownmdemu-core PUBLIC
        ${NAP_CORE_DIR}
        ${NAP_COMMON_DIR}
        ${NAP_M68K_DIR}/source/interpreter
        ${NAP_Z80_DIR}/source)
target_link_libraries(clownmdemu-core PRIVATE clown68000-interpreter clownz80-interpreter)

add_library(napsega_native_proof SHARED
        nap_sega_native_proof.cpp)

target_compile_features(napsega_native_proof PRIVATE cxx_std_17)
target_compile_definitions(napsega_native_proof PRIVATE
        NAP_SEGA_VENDOR_CORE_PRESENT=1
        NAP_SEGA_VENDOR_CLOWNMDEMU_OFFLINE=1
        NAP_SEGA_BUILD2QG=1)

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
