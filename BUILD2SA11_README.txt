BUILD2SA11_STAGE199_MEMORY_CARD_PER_GAME (1 soubor: nap_ps1_native.cpp, po SA6)

MEMORY CARD PER HRA (plan bod 1):
- Karta (SAVE_RAM jadra) se uklada do ps1_saves/<nazev_hry>.srm
- Nacteni pri bootu (MEMCARD_LOADED / MEMCARD_NEW pri prvnim hrani)
- Prubezny zapis ~5 s JEN pri zmene (otisk - setri flash) + pri stopu/vymene
- Hra si pozice najde vzdy sama. Hry z webu = kazda vlastni karta;
  rucni fd vyber sdili "rucni_vyber.srm" (jmeno nezname).
Markery: MEMCARD_LOADED / MEMCARD_NEW / MEMCARD_SAVED why=periodic|stop
OVERENO: ARM64 krizova kompilace + --no-undefined link OK.

TEST: hra z webu -> ULOZ ve hre na kartu -> MEMCARD_SAVED -> jina hra ->
zpet k prvni -> MEMCARD_LOADED a pozice tam je.

PLAN (schvaleno vc. tvych uprav): 2) interni/SD volba 3) joystick odezva
(napred merit) 4) externi gamepad PRO VSECHNY EMU 5) vlastni TV vystup
V APPCE pro vsechny emu i prehravac (zadna externi appka).
