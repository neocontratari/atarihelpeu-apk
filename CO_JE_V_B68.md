# B68 — ČISTÁ CESTA (versionCode 116)

## Nejdřív k tomu JPEGu, ať je to jasné

Já ho tam nedal — **já ho odtamtud vyhodil.** Byl v projektu dávno přede
mnou (`ps1FramePreviewB64`, značka BUILD2SK85) a stál 90 ms na snímek.
Ta moje věta znamenala: starou JPEG cestu jsem smazal a **zapomněl jsem
BIOSu dát náhradu**. To byla ta chyba, ne že bych JPEG někam přidával.

## Kde JPEG v projektu je a kde není — prohledáno celé

**Obrazovka telefonu: ani jednou.** Cesta obrazu je:

    jadro kresli v GPU -> sdilena textura -> OpenGL ES -> displej

Žádná komprese, žádný obrázek, žádný text.

**Jediný zbylý výskyt** je `napTvWebJpeg` — přenos k tobě do prohlížeče
přes Wi-Fi. Čte se na třech místech a všechna tři jsou obsluha HTTP pro
Chrome. Navíc se i tenhle přeskakuje, když běží H.264 stream.

Když chceš, aby PS1 nešel do prohlížeče přes JPEG **vůbec** (jen H.264),
řekni a udělám to — ale je to přenos na TV, ne kreslení na mobilu.

## Co jsem ještě smazal

Ve stránce zůstával mrtvý obrázek `#ps1Screen` i s obsluhou — pozůstatek
té staré cesty. Nic už do něj nechodilo, ale ležel tam. Pryč je i jeho
CSS a úklidový kód.

Ve stránce teď na JPEG ani base64 **nenajdeš jediný odkaz**.

## Opravy v tomhle buildu (všechny z minula platí)

1. **BIOS má konečně zobrazovací plochu.** Zapínala se jen pro hru, takže
   BIOS neměl čím kreslit. Teď se zapne i pro něj, a dřív než jádro.
2. **Obraz se četl neotočený** — renderer kreslí do paměti vzhůru nohama.
   Odtud to prolínání s intrem.
3. **Jádro vracelo neplatný povrch** — odtud `jas=0`, tedy černá.
4. **Smyčka běžela na 899 snímků za vteřinu** — odtud ten zvuk. Strop 62.
5. Plocha se nepřehazuje (nezruší se jí povrch) a když by přesto tři
   vteřiny kreslila černou, přepne se sama na záložní cestu.

## Přeloženo

| část | čím | chyb |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | **0** |
| C++ | `clang --target=aarch64` | **0** |
