# B220 — CD/ISO je knihovna her, MEMORY CARD ukazuje stav (versionCode 266)

Rene: "v ps1 mame tlacitko CD/ISO - je tam sice moznost prohledavat
slozite mobil a najit tu hru. Ale nesli by to udelat jako knihovnu
stazenych her s podobnou grafikou jako ma memory card? ... Kdyz najedu
na dotycnou hru, napise se SPUSTIT - SMAZAT."

Přesně tohle (SPUSTIT/SMAZAT, seznam stažených her) appka už uměla -
jenže na tlačítku MEMORY CARD (od B214). Po tom, co jsme v B215-B218
zjistili, jak skutečná PS1 paměťová karta funguje (jedna, trvalá,
automatická), se ukázalo, že "MEMORY CARD" pro výběr her je matoucí
název - CD/ISO ("vlož disk") je pro výběr hry mnohem přirozenější.

===============================================================================
 CO SE PROHODILO
===============================================================================

  CD/ISO (`btnCd`) - teď otevírá knihovnu stažených her (dřív dělalo
  `bootPs1Native()` = rovnou systémový výběr souboru). Panel přejmenován
  na "VYBRAT HRU (CD/ISO)". Uvnitř panelu přibylo tlačítko VYBRAT SOUBOR
  RUČNĚ V TELEFONU - stará funkčnost (`ps1PickGame` přes systémový
  picker) nezmizela, jen se stala druhou, ne první volbou - pro případ,
  že hra nebyla stažená přes appku samotnou (např. nahraná přes USB).

  MEMORY CARD (`btnMemory`) - teď ukazuje jednoduchý, srozumitelný stav
  SKUTEČNÉ paměťové karty (přes už existující `ps1MemCardInfo()`,
  postavené v B213): dává jádro paměť? existuje soubor na disku? povedl
  se poslední zápis? Žádné SPUSTIT/SMAZAT - o tu jednu, trvalou kartu se
  stará jádro samo (viz B218), tady se dá jen podívat, že funguje.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (76 zkontrolováno).
  `stranka_kontrola.py` (TV cesta) - 0 chyb.

  Nová jsdom simulace (test_ps1_cd_swap.js):
    - klik na CD/ISO otevře panel "VYBRAT HRU (CD/ISO)" se seznamem
      stažených her (ne prázdný/systémový picker rovnou)
    - VYBRAT SOUBOR RUČNĚ uvnitř panelu pořád zavolá skutečný systémový
      picker (`ps1PickGame`) - stará cesta nezmizela
    - klik na MEMORY CARD otevře panel se stavem karty, NE knihovnu her
    - text stavu obsahuje srozumitelné hlášky ("Jádro dává skutečnou
      paměť", "128 kB") sestavené z `ps1MemCardInfo()`
