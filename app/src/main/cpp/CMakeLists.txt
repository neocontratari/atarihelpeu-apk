cmake_minimum_required(VERSION 3.10.2)
project(napsega_native_proof)

add_library(napsega_native_proof SHARED
        nap_sega_native_proof.cpp)

find_library(log-lib log)

target_link_libraries(napsega_native_proof
        ${log-lib})
