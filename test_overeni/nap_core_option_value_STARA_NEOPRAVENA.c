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
  return nullptr;
}
