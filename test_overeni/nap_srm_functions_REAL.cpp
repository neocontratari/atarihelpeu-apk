static void nap_srm_set_path(const std::string &gamePath) {
  // BUILD2SB18: OPRAVA ARCHITEKTURY - Rene: "memory card nepracuje jak
  // na PS1... uz po vyjeti z emu si nepamatuje ulozenou pozici... presne
  // tak, ze je porad memory card zasunuta v PS1." Mel pravdu a ja jsem
  // predtim (B215-B217) resil SPATNY problem: myslel jsem si, ze kazda
  // hra potrebuje SVOJI VLASTNI kartu (aby se navzajem "neprepsaly"),
  // a delal jsem tomu podle nazvu slozky/souboru jedinecnou cestu.
  //
  // Skutecna PS1 to ale nedela takhle. Ma JEDNU fyzickou kartu (128kB),
  // porad zasunutou, a RUZNE HRY NA NI PIRODZENE KOEXISTUJI - kazda hra
  // si na karte zabere jen nekolik ze 15 "bloku", zbytek zustava pro
  // ostatni hry i pro BIOS Memory Card Manager, kdyz nabootujes uplne
  // bez disku. To, co blok patri ktere hre, resi FORMAT KARTY SAMOTNE
  // (hlavicka + tabulka bloku), ne appka zvenku - presne to uz dela
  // jadro (PCSX ReARMed) uvnitr Mcd1Data, jakmile mu dame SKUTECNOU,
  // porad stejnou kartu.
  //
  // Cesta k .srm souboru proto uz vubec nezavisi na tom, jaka hra (nebo
  // jestli vubec nejaka) prave bezi - je VZDY STEJNA, at nabootuje BIOS
  // samotny (Memory Card Manager bez disku) nebo jakakoli hra. `gamePath`
  // uz se nepouziva vubec (parametr zustava kvuli existujicim volanim
  // na trech mistech v kodu, at se nemusi menit i tam).
  (void)gamePath;
  g_srm_path = g_savedir + "/memory_card_1.srm";
}
static void nap_srm_load() {
  void *mem = retro_get_memory_data(0);
  size_t sz = retro_get_memory_size(0);
  if (!mem || !sz) { NAPLOG("BUILD2SA11 MEMCARD_NONE core nedava SAVE_RAM"); return; }
  // BUILD2SB17: NALEZENO VLASTNIM TESTEM (Rene: "2x mer, jednou rez") -
  // kdyz .srm soubor JESTE NEEXISTUJE (prvni hrani teto hry), tenhle kod
  // nechaval Mcd1Data tak, jak byl - tedy s OBSAHEM PREDCHOZI HRY, pokud
  // uz appka v tehle session nejakou hru hrala! Karta je globalni buffer
  // sdileny mezi vsemi hrami po celou dobu behu appky, ne neco, co jadro
  // samo vynuluje pri kazdem prepnuti hry. Bez explicitniho vymazani by
  // nova hra dostala cizi ulozenou pozici, i kdyz jeji VLASTNI .srm
  // soubor na disku spravne neexistuje. Overeno testem se dvema hrami
  // stejneho nazvu - viz test_overeni/test_end_to_end.cpp.
  memset(mem, 0, sz);
  FILE *f = fopen(g_srm_path.c_str(), "rb");
  if (f) { size_t rd = fread(mem, 1, sz, f); fclose(f);
    NAPLOG("BUILD2SA11 MEMCARD_LOADED %s bytes=%zu/%zu", g_srm_path.c_str(), rd, sz);
  } else NAPLOG("BUILD2SA11 MEMCARD_NEW %s size=%zu (prvni hrani teto hry, karta vynulovana)", g_srm_path.c_str(), sz);
  g_srm_last_fnv = nap_fnv32((const uint8_t*)mem, sz);
}
static void nap_srm_save_if_dirty(const char *why) {
  void *mem = retro_get_memory_data(0);
  size_t sz = retro_get_memory_size(0);
  if (!mem || !sz || g_srm_path.empty()) return;
  uint32_t h = nap_fnv32((const uint8_t*)mem, sz);
  if (h == g_srm_last_fnv) return;
  FILE *f = fopen(g_srm_path.c_str(), "wb");
  if (!f) { NAPLOG("BUILD2SA11 MEMCARD_SAVE_FAIL %s", g_srm_path.c_str()); g_srm_last_save_ok = 0; return; }
  fwrite(mem, 1, sz, f); fclose(f);
  g_srm_last_fnv = h;
  g_srm_last_save_ok = 1;
  g_srm_last_size = sz;
  NAPLOG("BUILD2SA11 MEMCARD_SAVED %s bytes=%zu why=%s", g_srm_path.c_str(), sz, why);
}
