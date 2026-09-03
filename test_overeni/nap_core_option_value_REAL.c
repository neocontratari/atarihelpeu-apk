static const char *nap_core_option_value(const char *key) {
  if (!key) return nullptr;
  if (strcmp(key, "pcsx_rearmed_bios") == 0) return "auto";
  if (strcmp(key, "pcsx_rearmed_show_bios_bootlogo") == 0) return "enabled";
  if (strcmp(key, "pcsx_rearmed_region") == 0) return "auto";
  if (strcmp(key, "pcsx_rearmed_drc") == 0) return "enabled";
  if (strcmp(key, "pcsx_rearmed_async_cd") == 0) return "async";
  // BUILD2SK94: REVERT (puvodni pokus) - podezreni na souvislost s rostoucimi
  // audio underruny. Nikdy to nebylo jiste - jen casova shoda.
  // BUILD2SK96: ZNOVU ZAPNUTO, izolovane. SK95 nasel a opravil SKUTECNOU
  // pricinu audio problemu (unik pameti v NativePs1InPlaceView - chybejici
  // bitmap.recycle() pri zmene PS1 rozliseni) - Rene potvrdil zvuk uz v
  // poradku, na telefonu i v prenosu. SK94uv podezreni na Enhanced
  // Resolution tedy nejspis bylo vedle - jen nahodna casova shoda se
  // skutecnym unikem pameti, ktery bezel soubezne. Rene poslal screenshoty
  // potvrzujici src=320x240 (nativni rozliseni teto konkretni hry/obsahu) -
  // presne scenar, pro ktery je Enhanced Resolution urcene (zdvojnasobi 3D
  // geometrii, ne 2D/FMV - cutsceny se timhle NEZLEPSI, to je jiny, trvaly
  // strop, viz PS1 rozliseni diskuze).
  // BUILD2SK142: SK141 VRACENO ZPET - enhanced rozliseni je presne DUVOD,
  // proc Rene presel ze softwaroveho (hranateho) vykreslovani na gpu-gles
  // vubec - vypnuti tim padem nebylo prijatelny kompromis k otestovani,
  // bylo to zruseni cele smysluplnosti teto vetve. Zustava ZAPNUTO,
  // nedotknutelne. Zbytek zvukoveho problemu se musi resit jinak - viz
  // nova cista diagnostika ve vlakne emulace (mereni bez zmeny chovani).
  if (strcmp(key, "pcsx_rearmed_neon_enhancement_enable") == 0) return "enabled";
  // BUILD2SB16: NALEZENA SKUTECNA PRICINA, PROC MEMORY CARD "NEFUNGOVALA".
  // Rene: "chci primou emulaci a bez fake... memory card u ps1 je dulezita,
  // bez toho to nema smysl." Mel pravdu, a bylo to hlouběji, nez jsem
  // cekal - nebyl to jen problem s kolizi nazvu (B215), jadro melo
  // memory kartu VYPNUTOU CELOU DOBU:
  //
  // PCSX ReARMed (load_memcards() v libretro.c) se pta na promennou
  // "pcsx_rearmed_memcard1" - a kdyz ji frontend NEUMI odpovedet (jako
  // dosud tady), jadro rovnou preskoci nastaveni karty a necha
  // memcard_type[0] na vychozich 0 = MEMCARDTYPE_NONE. Bez tohohle
  // retro_get_memory_data(RETRO_MEMORY_SAVE_RAM) VZDY vraci NULL a
  // retro_get_memory_size VZDY 0 - presne to, co uz NAPLOG hlasil jako
  // "MEMCARD_NONE core nedava SAVE_RAM", jen jsem to driv necetl jako
  // "jadro ma kartu vypnutou", ale jako neskodny okrajovy pripad.
  //
  // Hodnota "libretro" prepne jadro na REZIM, kdy memory kartu drzi ono
  // samo v pameti (Mcd1Data, 128 kB - presne velikost skutecne PS1
  // karty) a frontend (my) si ji jen ctyri/zapisujeme pres SAVE_RAM
  // API - presne mechanismus, ktery uz B215 opravil pro jedinecnost
  // souboru na hru. Bez teto radky byl cely ten system pripojeny na nic.
  if (strcmp(key, "pcsx_rearmed_memcard1") == 0) return "libretro";
  if (strcmp(key, "pcsx_rearmed_memcard2") == 0) return "libretro";
  return nullptr;
}
