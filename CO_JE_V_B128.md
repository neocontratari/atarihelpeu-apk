# B128 — TESTOVACÍ PLÁN PŘÍMO V APLIKACI (versionCode 176)

> **Tento kód je předpoklad. Čeká se na test.**
> **Nahrazuje B125, B126 i B127 — ty už nestav.**

## Měl jsi pravdu podruhé

Tabulku „co testovat / co je správně / co je špatně" jsem ti napsal — ale
do souboru **uvnitř ZIPu**. Ty přitom držíš v ruce telefon. Abys ji viděl,
musel bys rozbalit archiv na počítači a číst markdown. To není „mít se
čeho chytit", to je testování naslepo.

Teď je ten plán **na té obrazovce, kterou testuješ.**

## Jak to teď vypadá

Nahoře na obrazovce HELP je oddíl **CO TESTOVAT** a v něm sedm karet.
U každé:

```
3. ATARI 130XE EMULATOR - zahraj si chvíli
   SPRÁVNĚ: Běží ÚPLNĚ STEJNĚ jako dosud. Obraz, zvuk, ovládání, rychlost.
   SPATNE:  Jakákoli změna proti minulé verzi. Tohle je nejdůležitější krok.

   [ V POŘÁDKU ]   [ ŠPATNĚ ]
```

Klepneš na jedno z těch dvou tlačítek. Karta se obarví (zeleně vlevo, nebo
červeně) a **klepnutí se zapíše do logu**:

```
BUILD2SA14 TEST_KROK 3 vysledek=V_PORADKU co=ATARI 130XE EMULATOR - zahraj si chvili
BUILD2SA14 TEST_KROK 5 vysledek=SPATNE co=SPUSTIT TEST JADRA
```

Pod kartami je souhrn: *„V pořádku: 6   Špatně: 1   Zbývá: 0   >>> POŠLI MI
LOG, něco nesedí."*

## Co to znamená pro tebe

**Nemusíš nic psát.** Odklepeš sedm kroků, dole zmáčkneš ZKOPÍROVAT LOG DO
SCHRÁNKY a vložíš ho do zprávy. Tvoje výsledky jsou v tom logu.

Nemusíš ani hlásit čísla z testu jádra — ta jsou v logu taky.

## Ověřeno

Stránku jsem **spustil** proti napodobenině rozhraní, ne jen zkontroloval
syntax. Vykreslí sedm karet, čtrnáct tlačítek, klepnutí se zapíše správně
a souhrn počítá dobře.

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 40 tříd |
| C++ Atari, PS1, Sega (aarch64, `-Wall -Wextra`) | 4/4, 0 chyb, 0 varování |
| linkování | 31/31 + Atari 1/1 |
| JS rozcestníku i obrazovky HELP | 0 chyb |
| obrazovka HELP spuštěná v node | 7 karet, klepnutí zapsáno, souhrn sedí |
| původní Atari | `diff -rq`, bajt po bajtu netknuté |

## Zbytek z předchozích kol

- **Log je v aplikaci**, ne přes web viewer: výpis na obrazovce, přesné cesty
  k souborům, tlačítko do schránky.
- **Log přežije pád**: `Download / AtariHelp / PS1_LOG_predchozi.txt`.
  Cesta se píše do logu hned při startu aplikace.
- **Oprava 384 kB na zásobníku** v nativním testu (`AtariMem` byla lokální
  proměnná; mohlo to spadnout hned při prvním stisku).
- **Logování obrazovek** — v logu je vidět, kudy jsi při testu prošel.
- Původní Atari netknuté, nové C++ Atari na tlačítku HELP,
  kill-switch `NAP_ENABLE_ATARI OFF`.

---

## CO TESTOVAT

Všechno je na obrazovce HELP. Tady je to jen pro pořádek:

| # | co | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|---|
| 1 | build v Actions | zelený | červený → log z Actions |
| 2 | rozcestník | sedm tlačítek jako dosud | cokoli přeskládané |
| 3 | **ATARI 130XE** | běží úplně stejně jako dosud | jakákoli změna |
| 4 | tlačítko HELP | tmavá stránka „ATARI 130XE v C++" | starý panel nebo červený pruh |
| 5 | SPUSTIT TEST JÁDRA | zelený pruh „OBOJI SEDI" | červený pruh, chyba, pád |
| 6 | PS1 a SEGA | stejné jako dosud | ticho, trhání, jiný obraz |
| 7 | ZKOPÍROVAT LOG | hlásí „ZKOPIROVANO (N znaku)" | hlásí „NEPOVEDLO SE" |

**Krok 3 je nejdůležitější.** Původní Atari se nesmí změnit ani o chlup.

## CO POSLAT ZPĚT

Jen **log ze schránky**. Nic jiného, nic v něm nehledej.

Když to spadne, soubor `Download / AtariHelp / PS1_LOG_predchozi.txt`.
Když je build červený, log z GitHub Actions.

---

## Co hotové NENÍ

Vrstvy 3 až 6: ANTIC, GTIA, PMG, POKEY, VBXE, napojení obrazu.
Obraz ani zvuk na obrazovce HELP nejsou a do 27. srpna nebudou.
Beta pojede na původním Atari — tak, jak jsi to rozdělil.

## Nalezené chyby v původním JS (neopravené, nespěchají)

1. **Rozšířená paměť má místo 16 bank jen 4** — `& (ext.length-1)` u 320 kB
   zahodí bity 16 a 17. Banka 4 je fyzicky tatáž paměť jako banka 0.
2. **PMG registry se čtou z konce řádku, ne ze začátku** — `pmSeg` nemá
   snímek stavu jako `chSeg`. Dopad na Decathlonu: 8 bodů na jednom řádku.
3. **Rozejití atletů v Decathlonu — nenalezeno.** Čtyři hypotézy, tři
   vyvrácené, čtvrtá dělá 8 bodů. Hledám dál.
