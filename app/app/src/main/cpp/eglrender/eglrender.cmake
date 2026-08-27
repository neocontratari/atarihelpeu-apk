# ============================================================
#  KROK B: nas EGL renderer jako DRUHA nativni knihovna vedle
#  emulatoru. V teto verzi se JEN STAVI (overeni, ze se dva
#  nativni svety v jednom balicku snesou) - jeste se nespousti.
#  V kroku C ho napojime na zobrazovani PS1.
# ============================================================

# Oficialni "lepidlo" Androidu (soucast NDK) - pro pripad, ze by
# renderer bezel jako NativeActivity. V kroku B jen pro linkovani.
add_library(egl_native_app_glue STATIC
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(egl_native_app_glue PUBLIC
    ${ANDROID_NDK}/sources/android/native_app_glue)

# Nas renderer jako sdilena knihovna libeglrender.so.
# Zatim jen demo (barevne pruhy) - PS1 napojeni prijde v kroku C.
add_library(eglrender SHARED
    ${CMAKE_CURRENT_LIST_DIR}/egl_main.c
    ${CMAKE_CURRENT_LIST_DIR}/egl_logserver.c
    ${CMAKE_CURRENT_LIST_DIR}/egl_core_demo.c
    ${CMAKE_CURRENT_LIST_DIR}/core_ps1.c)

target_include_directories(eglrender PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(eglrender
    egl_native_app_glue
    android
    log
    EGL
    GLESv2
    OpenSLES
    dl
    m)

target_link_options(eglrender PRIVATE "-Wl,-u,ANativeActivity_onCreate")
# D3 build marker - opravena znacka (CMake pouziva # ne //)
