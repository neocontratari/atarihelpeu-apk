cmake_minimum_required(VERSION 3.22.1)
project(eglrender C)

# Oficialni "lepidlo" mezi Androidem a nasim C kodem (soucast NDK)
add_library(native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(native_app_glue PUBLIC
    ${ANDROID_NDK}/sources/android/native_app_glue)

# Nase knihovna - jmeno "eglrender" MUSI sedet s android.app.lib_name v manifestu
add_library(eglrender SHARED
    main.c
    logserver.c
    core_demo.c
    core_ps1.c)

target_link_libraries(eglrender
    native_app_glue
    android
    log
    EGL
    GLESv2
    dl
    m)

# KRITICKY RADEK: bez nej linker vyhodi vstupni bod ANativeActivity_onCreate
# (protoze na nej nic primo neodkazuje) a apka spadne hned pri startu
# s hlaskou "Unable to find native library". Klasicka zakerna chyba.
target_link_options(eglrender PRIVATE "-Wl,-u,ANativeActivity_onCreate")
