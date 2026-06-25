AtariHelp.eu EMU-10 / VBXE 130XE
BUILD2JT_PM_VISIBILITY_AUDIT_SAFE
Datum: 2026-06-25
Autor overlaye: ChatGPT pro Rene Frank / NaP Studio

CO JE TOHLE ZA BUILD
- Navazuje presne na potvrzeny posledni stav BUILD2JS_GTIA_PM_DMA_LATCH_SAFE.
- Tohle neni graficka oprava a neni to hack na jednu hru.
- Pridana je diagnostika GTIA/ANTIC Player-Missile pipeline do logu.
- Renderer, VBXE, XEX loader, kazeta, joystick, UI a funkce BUILD2JS zustavaji zachovane.

KODY JSOU STEJNE
- BASIC/Turbo BASIC testovaci kody nejsou menene.
- Altirra porovnavaci kody nejsou menene.
- V overlayi je meneny pouze app/src/main/assets/emu_vbxe/index.html kvuli PM audit logum.

PROC BUILD2JT
Posledni test BUILD2JS ukazal:
- Tetris VBXE: OK
- HL / Heartlight: OK
- Commando hra: OK, ale Arnold intro stale chybi
- Night Driver: auto stale neni videt
- Decathlon: bez zmeny
- Popeye: porad neni spolehlive posouzeny / nejspis bez zmeny
- Mission: bez zmeny

Zaver: dalsi krok nesmi byt slepe lepeni VBXE rendereru. Je potreba zjistit, jestli emulator:
1) vubec nacita P/M DMA data z pameti,
2) kresli P/M pixely mimo obraz,
3) skryva je spatnou GTIA prioritou,
4) pouziva spatny PMBASE nebo single/double line rezim,
5) nebo se P/M registry prepisuji paprskem v jine casti radku.

CO NOVY LOG VYPISUJE
Hledej v LOGu tyto znacky:
- BUILD2JT PM AUDIT START
- BUILD2JT PM REG
- BUILD2JT ANTIC W
- BUILD2JT GTIA W
- BUILD2JT PM DMA FIRST PLAYER
- BUILD2JT PM DMA FIRST MISSILE
- BUILD2JT PM DMA LINE
- BUILD2JT PM SUMMARY

Nejdulezitejsi radek je BUILD2JT PM SUMMARY. Vyznami:
- pDmaLines / mDmaLines = kolik radku melo aktivni player/missile DMA
- pNonzeroLines / mNonzeroLines = kolik radku melo skutecna nenulova P/M data
- pPixels / mPixels = kolik P/M pixelu se emulator pokusil kreslit
- pDraw / mDraw = kolik z nich opravdu proslo na obraz
- pBlockedPF / mBlockedPF = kolik jich zakryla GTIA priorita/playfield
- pOff / mOff = kolik bitu bylo mimo viditelne okno
- PMBASE / DMACTL / GRACTL / PRIOR = klicove registry pro dalsi opravu

PRESNY TEST PLAN
1) Rozbal ZIP pres GitHub Desktop stejne jako predchozi overlaye.
2) Spust aplikaci / emulator.
3) Over v LOGu startovni radek:
   AtariHelp.eu EMU-10 BUILD2JT_PM_VISIBILITY_AUDIT_SAFE pripraven
4) Test 1 - Night Driver:
   - Nahraj nightdriver_vbxe.xex.
   - Dojdi do mista, kde ma byt auto.
   - Nech bezet aspon 20-40 sekund.
   - Uloz LOG.
   - Napis, jestli auto porad neni videt.
5) Test 2 - The Activision Decathlon:
   - Nahraj The Activision Decathlon.xex.
   - Nech nabehnout obraz, kde je problem.
   - Uloz LOG.
6) Test 3 - Mission:
   - Nahraj mission.xex.
   - Nech nabehnout problemove misto.
   - Uloz LOG.
7) Rychla ochrana proti regresi:
   - Tetris VBXE musi zustat OK.
   - Heartlight / HL musi zustat OK a bez sedeho obdelniku.

CO POSLAT ZPATKY
Posli prosim:
- screenshot problemove obrazovky,
- cely LOG z BUILD2JT,
- kratkou vetu: Night Driver auto ANO/NE, Decathlon zmena ANO/NE, Mission zmena ANO/NE, Tetris VBXE OK/NE, HL OK/NE.

DULEZITE
BUILD2JT sam o sobe pravdepodobne Night Driver auto jeste neopravi. Jeho smysl je rict pravdu:
- pokud pNonzeroLines > 0 a pDraw = 0, problem je priorita/offscreen/render PM.
- pokud pNonzeroLines = 0, problem je PMBASE/DMACTL/GRACTL/pametovy fetch/timing.
- pokud pDraw > 0 a auto neni videt, problem muze byt barva/pozice/prekryti/VBXE overlay.

PRILOZENE TEST XEX

nightdriver_vbxe.xex
- size: 32047 B
- xexRecords: 36
- D640/D740-like operand hits: 14
- sha256: ae1c6f01e09a23e164cfdfa8bf7c3c74d000a66211358e6867567cfe3546f02c
- first records: $2000-$2014, $02E2-$02E3, $2000-$2091, $2092-$209E, $8000-$9E81, $02E2-$02E3, $2000-$24D2, $2800-$3FFF

The Activision Decathlon.xex
- size: 13809 B
- xexRecords: 4
- D640/D740-like operand hits: 6
- sha256: 7c570c74a422889fb053887a424adc52f8a15dd747ae1ab8a3e933ece2f9ab1f
- first records: $2020-$203A, $0244-$0244, $4000-$75C0, $02E2-$02E3

mission.xex
- size: 40325 B
- xexRecords: 30
- D640/D740-like operand hits: 2
- sha256: 3a716e93dc7047bb504107a0be14197b709a2fab61184d386276b92214123e32
- first records: $AD00-$AD0E, $02E2-$02E3, $A800-$ABFF, $AD00-$AD9E, $02E2-$02E3, $9690-$9E7B, $7F60-$8661, $7400-$7EA9
