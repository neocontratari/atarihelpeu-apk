# B70 — LOMENÉ ČÁRY (versionCode 118)

## Základ

Tenhle balíček **staví na B56**, tedy na stavu, který ti běžel. Renderer,
zvuk, spouštění her — všechno jako tam. Přidána je jediná oprava v kódu.

## Co je opravené

V `naples2_gl.c`, ve zpracování příkazů pro čáry:

```c
if (cmd >= 0x40 && cmd <= 0x5f) {   /* cary */
    ...
    list += 1 + len;                /* PEVNA delka - CHYBA */
```

**Lomená čára** (příkaz s bitem 3) má na PlayStation **proměnnou délku** —
kreslí se, dokud nepřijde ukončovací slovo. Renderer ji přeskakoval o pevný
počet slov, tím **ztratil krok v seznamu příkazů** a od té chvíle četl
souřadnice vrcholů jako příkazy.

Teď se lomená čára čte správně, až po ukončovací slovo, a kreslí se všechny
její úseky včetně přechodu barev.

**Poctivě: menu BIOSu to samo o sobě nespraví.** Ověřil jsem si to u sebe —
obraz je s touhle opravou i bez ní stejný, protože BIOS v tom menu lomenou
čáru nepoužívá. Je to ale skutečná chyba, která rozhodí seznam příkazů u
každé hry, která lomené čáry používá, a těch je dost.

## Kde je grafika BIOSu

Zúžil jsem to na jedno místo. Ta zelená kaše vzniká z útvarů, které BIOS
kreslí jako **patnáctibitovou texturu na stránkách 704 a 896**. Když ty
útvary vynechám, menu je úplně čisté (poslal jsem obrázek). Ale vynechat je
nejde — hry patnáctibitové textury běžně používají a rozbilo by to je.

Zbývá zjistit, proč referenční renderer z těch samých dat udělá přeliv
a tenhle z nich udělá kaši. To je poslední krok.

## Vyřazeno měřením (ať to nikdo neopakuje)

- texturové okno — vypnuto, obraz bajt po bajtu stejný
- staré zbytky v paměti — v paměti nic nechybí
- kopie uvnitř videopaměti — BIOS ani jednu nedělá
- chybějící data z GPU — vráceno zpět, změna 0,03 z 255

## Přeloženo

| část | čím | chyb |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | **0** |
| C++ | `clang --target=aarch64` | **0** |
| jádro PS1 pro ARM64 vč. assembleru | `aarch64-linux-gnu` | **0** |
| jádro + renderer | přeloženo **a spuštěno** na skutečném GLES2 | — |
