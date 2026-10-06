# Co je v B298

Tvůj log po B297 a tvoje slova: „první chyba u delších kódů z WAV to špatně a
neúplně vypisuje kód programu, do chvíle se kód rozsype – nicméně hra funguje
v Turbo BASICu. Při CLOAD se program nahraje, nicméně zvuk neodpovídá reálnému
Atari 130XE. CSAVE je zvuk reálný. Při nahrávání her z netu to někdy jde
a někdy ne. Pro WAV jsou hry, kde je potřeba podržet START+OPTION. U W3D
nepomůže ani vypnout a zapnout emu – hra je neustále načtená.“

## 1. Rozsypaný LIST po CLOAD

- Na počítači jsem to zkoušel s dlouhými programy, jak by je psal člověk na
  klávesnici Atari (7–8 kB, 56–68 záznamů na kazetě), v Atari BASICu
  i v Turbo-BASICu XL: LIST → CSAVE → WAV → CLOAD → LIST.
- Výsledek: výpis souhlasí se zdrojem znak po znaku, program v paměti je po
  CLOAD bajt po bajtu stejný, všechny záznamy mají správný kontrolní součet,
  při čtení 0 chyb. Chybu jsem tedy u sebe **nenašel**.
- Nic jsem kvůli tomu neměnil, protože jsem nevěděl, co opravovat.
  Potřebuju tvůj WAV (`WALLSULTIMATED (N&P Edition) House Book CSave.wav`).
  Rozeberu ho bajt po bajtu a zjistím, jestli je to v jádru, nebo v programu
  samém (např. ochrana proti LIST, nebo LIST až po RUN, když hra přepnula
  grafiku / znakovou sadu).
- Nový test: `test_core kazeta-dlouha basic|tbxl`.

## 2. Zvuk při CLOAD jako na skutečném 130XE

- Skutečné Atari při CLOAD z kazety nic „nepouští“. Datová stopa do televize
  nejde, slyšet je jen zvuk, který dělá sám počítač (POKE 65,0 ho vypne).
  Ověřeno v ROM 130XE: OS při čtení každého záznamu zapne POKEY
  (AUDC1/2/4 = $A8). Kanál 4 dá bzučení ~600 Hz, kanály 1+2 mají AUDF=0,
  tedy ~32 kHz, a to na skutečném Atari slyšet není.
- **Chyba u nás:** těch 32 kHz se při převodu na 44,1 kHz „přeložilo“ do
  slyšitelna jako pískot ~12 kHz, skoro stejně silný jako samotný bzukot.
  To je ten rozdíl proti skutečnému 130XE.
- **Oprava:** zvuk má teď pásmo jako televize. Úrovně POKEY se nejdřív
  zprůměrují na 8× hustší mřížku, pak jdou přes filtr (FIR, hrana 16 kHz)
  a teprve potom na 44,1 kHz. Změřeno: energie nad 11 kHz spadla
  z 28,4 % na 0,6 %, pískot zmizel. Pomáhá to i ostatním hrám
  (vysoké tóny už „nešumí“).
- Navíc teď start bit na kazetě znovu spustí časovače 3+4
  (asynchronní příjem POKEY). Bzučení je tak „drsné“ jako na skutečném stroji.
- **Stereo WAV:** druhý kanál (zvuková stopa kazety: hudba, hlas) teď hraje
  do televize, když se pásek točí. Přesně to dělá skutečný magnetofon
  (AUDIO IN). U mono WAV hraje jen zvuk počítače.

## 3. Hry na kazetě (START+OPTION)

- Appka pozná sama z prvního záznamu kazety, co na ní je: BASIC program
  (CLOAD), hra s bootem, nebo výpis BASICu jako text (ENTER "C:").
- **Hra s bootem:** po vložení WAV se Atari zapne s drženým START+OPTION (jako
  při zapnutí skutečného 130XE). OS pípne, appka sama stiskne RETURN, motor se
  rozjede a hra se nahraje a spustí. Disketa z D1: se přitom vysune, aby
  nebootovala ona.
- **Ručně jako na skutečném Atari:** při vypnutém Atari drž START (a OPTION)
  a zapni POWER. Jde to i obráceně: drž START+OPTION, POWER vypnout a zapnout.
  Dřív boot z kazety ručně vůbec nešel: při vypnutém Atari tlačítka
  nereagovala a při zapnutí se pouštěla. RETURN po pípnutí stiskne appka
  sama.
- POWER bez START = normálně BASIC (kazeta v magnetofonu sama nebootuje, jako
  u skutečného 130XE).
- Na šířku je výběr kazety v kolečku (KAZETA (WAV)).

## 4. Disketa W3D „pořád načtená“

- Disketa v mechanice zůstává i po vypnutí/zapnutí. To je správně, stejně se
  chová skutečná mechanika. Chybělo ale, jak ji vyndat.
- **ATR/DISK** s disketou v D1: teď nabídne:
  - **VYSUNOUT DISKETU**: Atari naběhne znovu s BASICem,
  - **VYMĚNIT DISKETU BEZ RESTARTU**: když hra chce další disketu nebo
    druhou stranu,
  - **VLOŽIT JINOU DISKETU A NABOOTOVAT**.
- Při POWER s disketou je pod přístrojem: „V D1: JE wolf3d.atr – VYSUNOUT:
  ATR/DISK“.
- **Opravená chyba navíc:** s disketou v D1: nefungovalo CLOAD/CSAVE. Kazeta
  šla přes zkratku pro disketu a OS dostal „timeout“ (ERROR 138). Teď jde
  kazeta vždy přes OS a POKEY.

## 5. Hry z netu „někdy jde, někdy ne“

Z tvého logu jsou tři příčiny:

1. **Certifikát atarihelp.eu:** telefon (Android 9) občas odmítne spojení
   („Trust anchor for certification path not found“) a o pár sekund později
   stejná adresa projde. atarihelp.eu běží za ochranou WEDOS na dvou IP
   adresách. Typicky jedna z nich nepošle mezilehlý certifikát. Chrome si ho
   dohledá sám, Java v Androidu ne.
   **Oprava:** appka ho teď dohledá stejně jako prohlížeč (adresa vydavatele
   je přímo v certifikátu) a celý řetěz znovu zkontroluje systémem telefonu.
   Nic se nepovolí naslepo. Když to i tak nesedí, spojení se odmítne a v logu
   je `B298 TLS odmítnuto … server poslal …`, takže budu vědět víc.
   Ověřeno testem na PC (5/5): neúplný řetěz se doplní a projde,
   certifikát od cizí CA se odmítne.
2. **Záložní cesty:** proxy.cors.sh už neexistuje (v logu pokaždé
   „Unable to resolve host“), takže je vyřazená. Zůstávají allorigins
   a corsproxy.io.
3. **Opožděná stažení:** stažení z dřívějšího kliknutí doběhlo až po návratu
   do HELP a hra (Donkey Kong) se spustila „sama“. Moon Patrol a Galactic Chase
   doběhly najednou a zaváděly se obě. Teď platí jen **poslední** vybraná hra,
   starší stažení se zahodí (v logu `B298 NET_HRY opožděné stažení …
   zahozeno`).

V jednom případě telefon nenašel ani adresu atarihelp.eu („Unable to resolve
host“). To byl výpadek sítě v telefonu a s tím appka nic neudělá.

## Log

- Vložení kazety: počet záznamů s **platným kontrolním součtem**, chyby rámce
  **jen uvnitř záznamů** (šum v mezerách mezi záznamy se počítá zvlášť, OS ho
  nečte) a druh kazety. Těch „14 chyb rámce“ u tvého CSAVE byl právě šum
  v mezerách; nevadil.
- Konec CLOAD: kolik bajtů OS opravdu přečetl a kolik chyb při čtení.

## Ověřeno na počítači

- Skutečný `nap_atari_native.cpp` se skutečnými vlákny: **56 kontrol, 0 chyb**,
  z toho 8 nových:
  - vysunutí diskety,
  - výměna bez restartu (paměť zůstala),
  - CLOAD s disketou v D1:,
  - boot hry z kazety (sám START+OPTION a RETURN),
  - stereo zvuková stopa,
  - D1: prázdná při bootu z kazety,
  - POWER bez START = BASIC,
  - ručně: START+OPTION držené a POWER = boot z kazety.
- Dlouhé programy přes kazetu (BASIC i TBXL), Acid800 stejně jako dřív (42),
  130XE 9/9, VBXE (Popeye, W3D) beze změny, samokontrola 29C55806 / D3949DC5.
- Spektrum zvuku při CLOAD před a po opravě: pískot 12,3 kHz pryč.
