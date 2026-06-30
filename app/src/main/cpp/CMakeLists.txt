cmake_minimum_required(VERSION 3.22.1)
project(napsega_native_proof LANGUAGES C CXX)

# BUILD2PZ: real ClownMDEmu-core import, but fixed so CMake does the clone/submodule step explicitly.
# Normal Sega UI stays. No separate C++ window. No fake gameplay.
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
set(CC_USE_C99_INTEGERS ON CACHE BOOL "Use C99/C++ fixed integer types for ClownMDEmu on Android" FORCE)

FetchContent_Declare(
        clownmdemu_core
        GIT_REPOSITORY https://github.com/Clownacy/clownmdemu-core.git
        GIT_TAG master
        GIT_SHALLOW FALSE
        GIT_PROGRESS TRUE
        GIT_SUBMODULES libraries/clown68000 libraries/clownz80
        GIT_SUBMODULES_RECURSE TRUE
)

FetchContent_GetProperties(clownmdemu_core)
if(NOT clownmdemu_core_POPULATED)
    FetchContent_Populate(clownmdemu_core)
endif()

# GitHub Actions sometimes fetches the parent repo but leaves submodules empty during Gradle/CMake.
# Force it here and fail with a useful message instead of a silent Gradle stack trace.
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY ${clownmdemu_core_SOURCE_DIR}
            RESULT_VARIABLE NAP_SUBMODULE_RESULT
            OUTPUT_VARIABLE NAP_SUBMODULE_OUT
            ERROR_VARIABLE NAP_SUBMODULE_ERR)
    message(STATUS "BUILD2PZ clownmdemu submodule result=${NAP_SUBMODULE_RESULT}")
    message(STATUS "BUILD2PZ clownmdemu submodule out=${NAP_SUBMODULE_OUT}")
    message(STATUS "BUILD2PZ clownmdemu submodule err=${NAP_SUBMODULE_ERR}")
endif()

if(NOT EXISTS "${clownmdemu_core_SOURCE_DIR}/source/clownmdemu.h")
    message(FATAL_ERROR "BUILD2PZ: clownmdemu-core source/clownmdemu.h missing after fetch")
endif()
if(NOT EXISTS "${clownmdemu_core_SOURCE_DIR}/libraries/clown68000/CMakeLists.txt")
    message(FATAL_ERROR "BUILD2PZ: clown68000 submodule missing after recursive fetch")
endif()
if(NOT EXISTS "${clownmdemu_core_SOURCE_DIR}/libraries/clownz80/CMakeLists.txt")
    message(FATAL_ERROR "BUILD2PZ: clownz80 submodule missing after recursive fetch")
endif()

add_subdirectory(${clownmdemu_core_SOURCE_DIR} ${clownmdemu_core_BINARY_DIR} EXCLUDE_FROM_ALL)
if(NOT TARGET clownmdemu-core)
    message(FATAL_ERROR "BUILD2PZ: CMake target clownmdemu-core was not created by vendor CMakeLists")
endif()

add_library(napsega_native_proof SHARED
        nap_sega_native_proof.cpp)

target_compile_features(napsega_native_proof PRIVATE cxx_std_17)
target_compile_definitions(napsega_native_proof PRIVATE
        NAP_SEGA_VENDOR_CORE_PRESENT=1
        NAP_SEGA_VENDOR_CLOWNMDEMU_FETCH=1
        NAP_SEGA_BUILD2PZ=1)

target_include_directories(napsega_native_proof PRIVATE
        ${clownmdemu_core_SOURCE_DIR}/source
        ${clownmdemu_core_SOURCE_DIR}/libraries/clowncommon
        ${clownmdemu_core_SOURCE_DIR}/libraries/clown68000/source/interpreter
        ${clownmdemu_core_SOURCE_DIR}/libraries/clownz80/source)

find_library(log-lib log)
find_library(android-lib android)

target_link_libraries(napsega_native_proof PRIVATE
        ${android-lib}
        ${log-lib}
        clownmdemu-core)
