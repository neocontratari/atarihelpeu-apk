# Co je v B310

Tvoje zpráva: „Teď najedeme na těžký úkol – celý emu atari vbxe 130 xe vymazat v té JAVĚ – a na správné
tlačítko přesunout emu atari 130xe vbxe c++ z HELPU na správné tlačítko v aplikaci … tohle je radikální
řez – protože vím, že emu atari v c++ funguje na 120 procent.“ Na otázku, co má dělat HELP, jsi vybral
**Nápověda**.

## 1. Menu

- **ATARI 130XE EMULATOR** spustí Atari 130XE v C++, tedy přístroj, který byl doteď pod HELP.
  Staré Atari už v appce nikde není.
- **HELP** (dole vpravo) otevře okno **NÁPOVĚDA**: co dělá které tlačítko v menu i na přístroji Atari,
  jak se otevře Sega a PS1, OPTIONS a tlačítko ZPĚT. Stejné okno otevře i MENU vlevo nahoře
  (to tak bylo i dřív).
- OPTIONS → **TV POKUS** přepíná jen OBOJÍ / JEN ZVUK / JEN OBRAZ. Čtvrtá volba KRESLENÍM byla jen pro
  staré Atari a Java ji stejně nikdy nezapnula.

## 2. Co je smazané

| co | kde | velikost |
|---|---|---|
| staré Atari v JavaScriptu | `assets/emu_vbxe` | 524 kB (7245 řádků) |
| přesměrování na něj | `emu/`, `emulator/`, `emu1/`, `emu_1/` | malé |
| 9 starých záloh jeho stránky | `emu/` | 2,6 MB |
| ještě starší Atari | `emu10_130xe.html` | 132 kB |
| obrázky jeho klávesnice a joysticku | `design/vbxe_130xe_*_exact.png` | 3,7 MB |

Z appky tak zmizelo asi 6,9 MB souborů.

Java (MainActivity) – smazáno všechno, co sloužilo jen starému Atari (soubor má o 244 řádků méně):
- zvuk starého Atari na TV (`pushAtariPcm16`, fronta a vlákno `atariZvuk*`),
- vkládání her do jeho stránky (`EMU_URL`, `pendingGame`, `AHRECV_*` a 5 metod s čekáním a pokusy),
- výběr kazety a TXT pro něj (`pickAudio`, `pickText`, `injectText`) a ukládání souborů z něj (`saveBase64`),
- zastavování jeho zvukového vlákna, TV pokus „kreslením“, zmínky v seznamech obrazovek.

Staré Atari zůstalo jen v gitu, kdyby bylo potřeba:
`git show 7b40d21:app/src/main/assets/emu_vbxe/index.html`.
ROM v appce jsou z tvého skutečného Atari 130XE a v C++ zůstaly (`nap_atari_roms.h`).

## 3. Všechny hry Atari jdou do C++

- **NET HRY** na přístroji fungují jako dosud.
- **Hra z webu atarihelp.eu** (ATARIHELP.EU v menu → klik na hru) dřív skončila ve starém Atari.
  Teď se otevře Atari 130XE a hra se spustí v něm.
- **„Vybrat hru z telefonu“** na chybových stránkách webu: dřív starého Atari, teď výběr pro Atari 130XE.
- **Chyba stažení hry** (například místní server na Wi-Fi) se teď hlásí vždy, dřív jen z NET HRY.

## 4. Odemčení Segy (tohle by se jinak rozbilo)

Sega se otevírá po 50 napsaných řádcích programu v Atari. Ty řádky počítalo **jen staré Atari**
v JavaScriptu. Po smazání by se Sega novému hráči už nikdy neotevřela. Počítání je teď v C++:
- **Co se počítá:** řádek začínající číslem (řádek programu, např. `10 PRINT A`) = +1. Samotné číslo
  (smazání řádku) ani příkaz bez čísla se nepočítá.
- **Kdy se započítá:** dávka při RUN nebo LIST (i zkratka L.). Java ji uzná od 5 řádků, stejně jako dřív.
- **Odkud se počítá:** nově i psaní na dotykové klávesnici přístroje (staré Atari počítalo jen fyzickou
  klávesnici a klávesnici z TV / PC). Backspace řádek opraví, BREAK ho zruší.
- **Co se nepočítá:** text vložený z PC (Ctrl+V) a program z TXT souboru (ten píše appka sama).
- Při odemčení se na přístroji ukáže **SEGA ODEMČENA! JE V MENU**.

## Ověřeno na PC

- **Postup hráče v C++:** 21/0. Skutečný `nap_atari_native.cpp`, prstem po klávesnici přístroje,
  z klávesnice PC, Ctrl+V, BACK S, BREAK, L. a odchod a návrat do Atari.
- **NapPostup** (skutečná třída z appky, JVM): 10/0. Dávka pod 5 se nepočítá, Sega se otevře přesně na 50.
- **Hlavní nabídka v Chromiu** (přesně soubor z appky): 14/0.
  - ATARI 130XE EMULATOR vede na C++ Atari.
  - HELP i MENU otevřou nápovědu.
  - TV POKUS je bez KRESLENÍM.
  - Zamčená Sega hlásí „NAPIŠ JEŠTĚ 45 ŘÁDKŮ“.
  - Stránka Atari je bez chyby JavaScriptu.
- **Mosty JavaScript → Java:** 94 volání ze stránek a z HTML v Javě, všechna mají svou metodu v Javě.
- **Odkazy:** 63 odkazů v HTML a 11 cest k souborům v Javě a C++. Nic nevede na smazané soubory.
- **Java bez Androidu:** srovnání překladu B309 a B310 nenašlo žádnou novou chybu ve vlastním kódu
  (chybějící metoda nebo proměnná). Zkouška s úmyslně rozbitým kódem ji chytí.
- **Regrese Atari 130XE (C++):**
  - celý přístroj (test z B292): 86/0,
  - jádro (klávesnice PC, vkládání, proud na TV): bez chyb,
  - TV / PC v Chromiu se stránkou z B310: klávesnice PC 23/0, Atari na TV + Ctrl+V 23/0.

Postup: `test_overeni/b310/JAK_SPUSTIT.txt`, výstupy `test_overeni/b310/VYSTUP_*`.

**Co jsem ověřit nemohl:**
- Překlad Javy s Androidem. Na PC není Android SDK, to dělá CI build.
- Skutečný telefon:
  - že tlačítko ATARI otevře přístroj (stránka je stejná jako dřív pod HELP, Java ji pozná podle adresy),
  - hry z webu atarihelp.eu ve WebView,
  - výběr souboru,
  - hláška SEGA ODEMČENA.

## Log

- `B310 ATARI_HRA -> Atari 130XE (C++): …` – každá hra Atari, odkud přišla.
- `B310 POSTUP HRACE: davka N radku programu (RUN)` – C++ odeslal dávku.
- `B310 POSTUP_HRACE ATARI_RADKY +N -> radku=…/50 sega=…` – Java ji započítala;
  při odemčení `*** SEGA PRAVE OTEVRENA ***`.
- `B310 VYBER_HRY_Z_TELEFONU` – výběr hry z chybové stránky webu.
