cmake_minimum_required(VERSION 3.22.1)
project(napsega_native_proof)

add_library(napsega_native_proof SHARED
        nap_sega_native_proof.cpp)

target_compile_features(napsega_native_proof PRIVATE cxx_std_17)

# BUILD2PW core slot:
# When a real native Sega core source is added under app/src/main/cpp/vendor,
# replace the missing-vendor adapter with real source files and enable:
# target_compile_definitions(napsega_native_proof PRIVATE NAP_SEGA_VENDOR_CORE_PRESENT=1)
# Do NOT fake gameplay while this is 0.
target_compile_definitions(napsega_native_proof PRIVATE NAP_SEGA_VENDOR_CORE_PRESENT=0)

target_link_libraries(napsega_native_proof android log)
