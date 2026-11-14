#!/bin/bash
# B322: menu N&P pro web atarihelp.eu - PRESNE ten samy C++ jako v appce (app/src/main/cpp/menu),
# prelozeny Emscriptenem (3.1.x, apt install emscripten) do WebAssembly. Vse v jednom souboru
# nap_menu_web.js: WebAssembly + obrazky karet (assets/menu) + logo N&P + karty.txt.
# Pouziti: bash web/menu/vyrob.sh [vystupni slozka]      (vychozi web/menu/vystup)
set -e -o pipefail
TU="$(cd "$(dirname "$0")" && pwd)"; R="$(cd "$TU/../.." && pwd)"; VEN="${1:-$TU/vystup}"
A="$R/app/src/main/assets"
mkdir -p "$VEN"
em++ -O3 -std=c++17 -fno-exceptions -fno-rtti "$TU/nap_menu_web.cpp" -o "$VEN/nap_menu_web.js" \
  -sENVIRONMENT=web -sMIN_WEBGL_VERSION=1 -sMAX_WEBGL_VERSION=2 -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=33554432 \
  -sSINGLE_FILE=1 -sEXIT_RUNTIME=0 -sFILESYSTEM=1 -sTEXTDECODER=2 \
  -sEXPORTED_FUNCTIONS=_main,_napPanel,_napVelikost,_napViditelny,_napDotyk,_napKlavesa,_napPust,_napKarta,_napPocet \
  -sEXPORTED_RUNTIME_METHODS=ccall,cwrap \
  --embed-file "$A/menu@/assets/menu" \
  --embed-file "$A/design/nap_logo_original.png@/assets/design/nap_logo_original.png"
ls -la "$VEN/nap_menu_web.js"
sha256sum "$VEN/nap_menu_web.js"
