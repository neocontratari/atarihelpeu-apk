cmake_minimum_required(VERSION 3.22.1)
project(napsega_native_proof)

# BUILD2PY: real ClownMDEmu-core first boot attempt in normal Sega UI.
# GitHub Actions / Android Studio will fetch the official native C core and submodules during CMake configure.
# No ROM is shipped in APK. If fetching/build fails, send the CMake/NDK log; do not test a broken APK.
include(FetchContent)

set(CC_USE_C99_INTEGERS ON CACHE BOOL "Use C99/C++ fixed integer types for ClownMDEmu on Android" FORCE)

FetchContent_Declare(
        clownmdemu_core
        GIT_REPOSITORY https://github.com/Clownacy/clownmdemu-core.git
        GIT_TAG master
        GIT_SHALLOW FALSE
        GIT_SUBMODULES_RECURSE TRUE
)

# FetchContent_MakeAvailable will add target "clownmdemu-core" from the vendor CMakeLists.
FetchContent_MakeAvailable(clownmdemu_core)

add_library(napsega_native_proof SHARED
        nap_sega_native_proof.cpp)

target_compile_features(napsega_native_proof PRIVATE cxx_std_17)

# BUILD2PY: this means the real vendor source is linked into the native .so.
target_compile_definitions(napsega_native_proof PRIVATE
        NAP_SEGA_VENDOR_CORE_PRESENT=1
        NAP_SEGA_VENDOR_CLOWNMDEMU_FETCH=1)

target_include_directories(napsega_native_proof PRIVATE
        ${clownmdemu_core_SOURCE_DIR}/source
        ${clownmdemu_core_SOURCE_DIR}/libraries/clowncommon
        ${clownmdemu_core_SOURCE_DIR}/libraries/clown68000/source/interpreter
        ${clownmdemu_core_SOURCE_DIR}/libraries/clownz80/source)

target_link_libraries(napsega_native_proof
        android
        log
        clownmdemu-core)
