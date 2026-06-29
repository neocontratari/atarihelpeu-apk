BUILD2OI_SEGA_EU_BRANCH_BUTTON_STAGE47

CIL:
- nechat USA/World vetev stabilni
- pridat samostatnou EU/PAL vetvi bez rozbiti USA her
- priprava pro budouci realny PAL/EU core a pro grafiku, kterou doda uzivatel

KAM KLIKNOUT:
- Na hlavni Sega obrazovce klikni na panel vlevo dole u vlozene cartridge:
  "16-BIT / POWERED BY SEGA".
- Tim se otevre app/src/main/assets/emu_sega_eu/index.html
- Na EU vetvi stejny panel vraci zpet do USA vetve.

TEST USA VETEV:
1) Otevri normalni Sega.
2) Vyber Aladdin USA nebo Asterix USA.
3) Ocekavani: hra najede jako predtim, zvuk zustane BUILD2OD/2OH clean stable.

TEST EU VETEV:
1) Z normalni Sega klikni na panel 16-BIT / POWERED BY SEGA.
2) Otevre se EU/PAL testovaci vetev.
3) Vyber Aladdin Europe nebo Asterix Europe.
4) Ocekavani pro tento scaffold: v logu musi byt BUILD2OI_SEGA_EU_PAL_BRANCH_STAGE47 a region EU BRANCH -> FORCE EUROPE / PAL / International.
5) Pokud hra stale ukaze PAL/SECAM hlasku, je to potvrzeni, ze potrebujeme v EU vetvi realny PAL core/register, ne dalsi prepis wrapper configu.

CO POSLAT ZPET:
- USA vetev: Aladdin USA jede/ne, Asterix USA jede/ne
- EU vetev: Aladdin Europe jede/ne, Asterix Europe jede/ne
- screenshot pokud EU stale hlasi PAL/SECAM
- LOG z USA vetve a LOG z EU vetve
