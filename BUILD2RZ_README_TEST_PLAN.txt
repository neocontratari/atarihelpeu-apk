BUILD2RZ_SEGA_UI_LANDSCAPE_PAD_V2_COLLECTION_LINK_PS1_DIAG_STAGE142

DULEZITE: TENTO BUILD SE NEDOTYKA C++, JAVY, CMAKE ANI VENDOR JADRA.
Cerstvou S8 vyhru z RY (O3 + VDP) nechavam zamcenou - meni se JEN HTML/JS.
Zvukova latence v Noxu (0.8s po cinknuti) je zamerne ODLOZENA do dalsiho buildu,
protoze saha do audio cesty a zaslouzi si vlastni izolovany krok.

=========================================================
CO JE V TOMTO BUILDU (3 veci, vsechny HTML/JS):
=========================================================

1) SEGA - LANDSCAPE OVLADAC V2 (emu_sega/index.html)
   Proc byl "hnusny": zony se pocitaly z pozic PORTRAIT skin tlacitek, ktere
   v landscape sedely uplne jinde, kruhy byly male a dpad umel jen 4 smery
   bez diagonal (Sonic se nemohl koulet v behu).
   Nove:
   - Zony jsou z pevnych procent obrazovky: dpad vlevo dole, A/B/C oblouk
     vpravo dole jako na realnem MD padu. Velke, palcove.
   - DIAGONALY: UP/DOWN/LEFT/RIGHT jsou nezavisle bity (bezet + koulet zaroven).
   - POLOPRUHLEDNY OVERLAY: v landscape je videt kriz dpadu a tri krouzky
     s popisky A B C; pri stisku se rozsviti. Vzhled je zamerne decentni v1 -
     Rene jako designer si muze prestylovat (CSS tridy .lpRing/.lpBtnV/.lpCross).
   - MAPOVANI 1:1: krouzek "A" posila Genesis A, "B" -> B, "C" -> C.
     Overitelne v logu: INPUT POST_TO_NATIVE_CPP ... A1/B1/C1.
   - Portrait ovladani je BEZE ZMENY.

2) SEGA - TLACITKO SBIRKA (emu_sega/index.html)
   - Kratky tap: ulozi log COLLECTION_CLICK, cisto zastavi native hru
     a otevre https://atarihelp.eu/?page_id=207 (herni kolekce).
   - Dlouhy stisk 700 ms: puvodni SEGA VYKON menu (LOW/HIGH) - nic se neztraci.
   - Samostatne perf hit-zony perfHigh/perfLow funguji dal.

3) PS1 - DIAGNOSTIKA + LOG (emu_ps1/index.html)
   NAPRED POCTIVE VYSVETLENI, PARTAKU:
   Ty ctyri soubory scph*.bin NEJSOU emulacni core - jsou to BIOSy konzole
   (firmware od Sony, kazdy 512 KB). BIOS je "system PlayStationu", ktery
   emulator POTREBUJE, ale sam o sobe nic neemuluje. Emulacni core (obdoba
   ClownMDEmu pro Segu) je samostatny program a ten zatim v projektu neni.
   Takze: ZADNY fake PS1 boot ti nedam. Co ti davam uz TED:
   - CD/ISO a LOAD GAME tlacitka otevrou picker a udelaji REALNOU analyzu:
     * BIOS: rozpozna model + region. Tabulka otisku je overena proti tvym
       souborum: SCPH-1001 (USA 2.2), SCPH-5500 (JAPAN 3.0),
       SCPH-5501 (USA 3.0), SCPH-7502 (EUROPE 4.1).
     * PS1 disk (BIN/ISO/IMG): pozna RAW 2352B sektory, ISO9660 (CD001),
       Sony licencni sektor vc. regionu (USA/EU/JAPAN), SYSTEM.CNF a BOOT radek.
   - LOG pro testy: kazda akce se pise do logu; v SETTINGS je LOG panel
     + tlacitko KOPIROVAT LOG (posles mi ho jako text).
   - Design 1:1 obrazovky NEDOTCEN (zadne nove viditelne tlacitko).
   - BIOS ani hry se NIKDY nedavaji do APK - stejne pravidlo jako u Sega ROM.
   Dalsi realny krok pro PS1 core: vyber a integrace skutecneho jadra
   (napr. offline vendor stejnym stylem jako ClownMDEmu). To je vetsi prace
   na samostatnou vetev - reknu si o zeleneou, az bude Sega uplne dorazena.

ZMENENE SOUBORY:
- app/src/main/assets/emu_sega/index.html
- app/src/main/assets/emu_ps1/index.html
(Nic jineho. C++/Java/CMake/vendor z RY zustava.)

=========================================================
TEST PLAN (po lopate):
=========================================================
0) R4 OPRAVA DVOU PADU (moje chyba, priznavam):
   Landscape ovladac uz ve skinu EXISTOVAL (kruh s krizem vlevo + A/B/C sloupec
   vpravo) a ja k nemu v R1-R3 pridal druhy. R4 muj duplikat KOMPLETNE odstranuje
   a predelava ten puvodni podle tveho zadani:
   - dpad: ZADNY kriz - thumbstick PUNTIK, ktery klouze pod palcem,
     mala mrtva zona (7 %), diagonaly zustavaji
   - A/B/C: zustavaji V PRIMCE VPRAVO NAD SEBOU (jak jsi chtel), hezci vzhled,
     pri stisku se rozsviti zlute
   - mapovani 1:1: horni=A, prostredni=B, spodni=C (overitelne v INPUT logu)
1) SEGA landscape: otoc telefon/Nox na sirku pri hre.
   - MUSI byt videt JEN JEDEN ovladac: kruh s puntikem vlevo, A/B/C sloupec vpravo.
   - Puntik musi klouzat pod palcem; tlacitka se pri stisku rozsviti.
   - Zkontroluj popisky vs. akce ve hre (INPUT log ukaze A1/B1/C1).
2) SBIRKA: kratky tap -> otevre se atarihelp.eu kolekce (hra se korektne
   zastavi). Dlouhy stisk -> SEGA VYKON menu.
3) PS1: otevri PS1 stranku.
   - CD/ISO -> vyber scph1001.bin -> toast musi rict "SCPH-1001 (USA...)".
   - LOAD GAME - VYBER HRY -> vyber Medal of Honor .bin (ten VELKY 742MB soubor,
     NE .cue) -> toast s PS1 DISK detekci vc. regionu. Hra se zapamatuje.
   - Kdyz omylem vyberes .cue (80 bajtu): stranka ho precte a rekne ti,
     ktery .bin k nemu patri.
   - POZOR: Crash Bandicoot mas jako .7z - ten musis NAPRED ROZBALIT
     (7z nejde v appce cist primo; stranka te na to upozorni).
   - SETTINGS -> LOG: nove tlacitko "ULOZIT LOG (Downloads/AtariHelp)" -
     ulozi soubor AtariHelp_PS1_LOG_*.txt do STEJNE slozky jako Sega logy
     (zadny chaos). KOPIROVAT LOG zustava taky.
   - CERNY MONITOR NA PS1 STRANCE JE SPRAVNE: core neni pripojen,
     obraz se rozsviti az s realnym jadrem. Zadny fake boot.

LOG MARKERY:
- BUILD2RZ_SEGA_UI_LANDSCAPE_PAD_V2_COLLECTION_LINK_STAGE142
- LANDSCAPE_PAD_V2_READY zones=screenPercent diagonals=YES overlay=YES
- COLLECTION_CLICK url=... / COLLECTION_LONGPRESS perfMenu
- BUILD2RZ_PS1_DIAG_BIOS_ISO_LOG_STAGE142_R3 + LOG_SAVE DOWNLOADS_OK + BIOS_DETECT / DISC_DETECT / CUE_PARSED radky

CO SE NESMI STAT:
- Portrait ovladani se nesmi zmenit.
- Hra na S8 musi hrat stejne jako v RY (zadny zasah do zvuku/jadra).

ODLOZENO (vedome, ne zapomenuto):
- Nox 0.8s prodleva efektu: dalsi build (RZ2/SA) - zmensime FIFO target a
  AudioTrack buffer, ted na to mame diky O3 jadru rezervu. Chce vlastni test.
- Levy pruh: pridame pasivni marker, ktery ukaze, jestli Sonic VDP blank flag
  vubec zapina. Jestli ne, pruh je autenticke chovani Mega Drive, ktere na CRT
  televizich schovaval overscan - pak ti nabidnu volitelny "CRT orez" 8 px.

=========================================================
PS1 JADRO - PLAN (zelena od Reneho 3.7.2026):
=========================================================
Vybrane jadro: PCSX-ReARMed (libretro), overeno: GitHub libretro/pcsx_rearmed,
~18 MB zdrojaku, licence GPL-2.0, delane primo pro slabsi ARM telefony
(dynarec = rychly preklad PS1 procesoru), idealni pro S8.
Pozn. k licenci: GPL-2.0 vyzaduje dostupnost zdrojaku appky s jadrem - repo je
na GitHubu, takze OK, jen to budeme uvadet.

ETAPY (stejna disciplina jako u Segy - jedna vec, jeden log, jeden test):
- SA1: offline vendor import jadra + CMake + kompilace .so v Actions.
       Log marker: PS1_CORE_COMPILED. ZADNY obraz jeste.
- SA2: BIOS boot - Java posle scph1001.bin do C++, jadro nabootuje BIOS,
       prvni realny snimek (PS logo) pres stejny TextureView jako Sega.
       Tvuj test: PS logo na obrazovce = "SEGA moment" pro PS1.
- SA3: CD/ISO streaming - 700MB .bin NEJDE cist pres base64 jako Sega ROM;
       Java preda nativnimu kodu file descriptor a jadro cte sektory primo
       z disku. Boot Medal of Honor / Crash.
- SA4: zvuk (SPU do stejne audio cesty jako Sega FIFO) + ovladani + memory card.
Kazda etapa = samostatny overlay ZIP s test planem. BIOS/hry NIKDY v APK.
