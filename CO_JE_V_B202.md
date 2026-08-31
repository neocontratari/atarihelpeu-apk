# B202 — JS z emu_ps1 teď píše i do /8765/log (versionCode 248)

Reného poznámka: appendNativeLog() (co servíruje /8765/log) slyší jen
Javu. Cokoli se dělo v JS na stránce (včetně celé nové editace rozložení
tlačítek z B201), nebylo v /8765/log vidět vůbec - jen v panelu přímo na
telefonu, který Rene vidí, ale já ne.

===============================================================================
 CO SE ZMĚNILO
===============================================================================

  MainActivity.java, třída AHPS1 - nová metoda:

      @JavascriptInterface
      public String ps1Log(String line) {
          appendNativeLog("PS1_JS " + (line == null ? "" : line));
          return "OK";
      }

  emu_ps1/index.html - nová pomocná funkce jsLog(t), která dělá OBOJÍ:
  zapíše do panelu na telefonu (jako dřív log(t)) a zavolá
  window.AHPS1.ps1Log(t), takže řádek doputuje i do /8765/log.

  Napojeno na klíčové kroky editace rozložení:
      LAYOUT_EDIT_ENTER, LAYOUT_DRAG_START, LAYOUT_DRAG_END, LAYOUT_RESET,
      LAYOUT_EDIT_EXIT, LAYOUT_SAVE (s celým uloženým rozložením v JSON),
      LAYOUT_LOAD (co se při startu stránky našlo nebo nenašlo).

  Prefix "PS1_JS" v /8765/log odlišuje řádky ze stránky od řádků, co si
  Java loguje sama - snadno se to bude dát najít/filtrovat.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check na emu_ps1/index.html - 0 chyb.
  Ruční kontrola rovnováhy složených závorek v MainActivity.java (javac
  bez Android SDK spustit nejde) - 2288 otevíracích, 2288 zavíracích, sedí.

  V jsdom simulaci (stejný test jako v B201, rozšířený) jsem ověřil, že
  window.AHPS1.ps1Log() se volá přesně u těch kroků, kde má, a že obsahuje
  čitelný, užitečný text (ne jen "OK" nebo prázdno) - včetně kompletního
  JSON rozložení při LAYOUT_SAVE a LAYOUT_LOAD, aby šlo z jednoho řádku
  logu poznat přesně, kam byla která tlačítka přesunuta.

===============================================================================
 DO BUDOUCNA
===============================================================================

  Stejný vzor (jsLog nebo obdoba) použiju od teď u KAŽDÉ nové JS funkce,
  kterou přidám do libovolné stránky appky - ne až když si toho Rene
  všimne. Cíl: v /8765/log má být vidět všechno, co appka dělá, ne jen
  polovina.
