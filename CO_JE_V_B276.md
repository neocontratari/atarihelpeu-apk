# B276 — Dvoutónové kódování dat při CSAVE

## Co jsem hledal

Ze skutečného logu bylo jasně vidět: appka hraje po celých 42 vteřin
**úplně stejný, konstantní tón** (AUDF=[5,7,204,5]), a pak najednou
ztichne. Žádná změna, žádné "chrčení" — přesně jak jsi popisoval.

## Co jsem zjistil

Přímo jsem ověřil dvě věci:
- **Timer4 přerušení** (co bych čekal jako spouštěč "bit po bitu"
  přepisování zvuku) je po celou tuhle fázi **úplně vypnuté**
- **Reference (JS)** používá pro syntézu zvuku jen obyčejné
  AUDF/AUDC/AUDCTL — nikde ani zmínka o sériovém bitu nebo
  dvoutónovém režimu

Závěr: skutečný POKEY hardware si v "dvoutónovém" režimu (bit3
registru SKCTL — přesně ten bit, co jsem dřív našel jako rozlišující
mezi CSAVE a CLOAD) **sám v křemíku** vybírá mezi dvěma frekvencemi,
podle aktuálního bitu v posuvném registru sériových dat. Tenhle
mechanismus appce v mé emulaci úplně chyběl.

## Oprava

Při zápisu do SEROUT appka teď naloží 10bitový sériový rámec (start
bit + 8 datových bitů + stop bit), časovaný podle 600 baudů.
Generování zvuku dělí výstup na úseky podle jednotlivých bitů — pro
bit=0 použije dvojnásobnou frekvenci (poloviční periodu spojeného
kanálu 3+4), přesně jak to dělá skutečný hardware.

## Poctivé přiznání z vývoje

Můj úplně první izolovaný test dával zmatené, "obrácené" výsledky —
ukázalo se, že chyba byla v samotném testu (špatně jsem simuloval
časování mezi zápisem dat a generováním zvuku), ne v logice appky.
Po opravě testu jsem přímo změřil poměr frekvencí bit=0/bit=1: **1,91**
— velmi blízko očekávaným 2,0 pro skutečné dvoutónové kódování.

## Ověření

- Poměr frekvencí přímo změřen: 1,91 (očekáváno ~2,0)
- Všech 9 předchozích testů prochází beze změny
- CLOAD pořád správně končí ERROR 138 — beze změny

## Co zůstává k ověření

Jak to doopravdy **zní** na živém uchu — jestli to teď připomíná to
originální "chrčení" — může potvrdit jen test na tvém skutečném
130XE.
