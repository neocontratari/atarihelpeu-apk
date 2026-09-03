static void nap_srm_set_path(const std::string &gamePath) {
  // BUILD2SB15: Rene si vsiml rizika - kdyz maji dve RUZNE hry stejny
  // nazev souboru (bezne u PS1 dumpu - "game.bin", "disc1.bin"...), drive
  // se pouzival JEN nazev souboru jako klic k ulozene pozici. Dve ruzne
  // hry se stejnym nazvem by si tise prepsaly memory kartu navzajem,
  // BEZ JAKEHOKOLI varovani. Kazda hra uz ale ma svou vlastni slozku
  // (viz ps1RemoteCacheDir v Jave) - ta je VZDY jedinecna. Klic pro
  // ulozenou pozici proto skladame ze slozky I nazvu souboru, ne jen
  // z nazvu souboru samotneho.
  std::string leaf = gamePath;
  size_t sl = leaf.find_last_of('/');
  std::string parentDir;
  if (sl != std::string::npos) {
    leaf = gamePath.substr(sl + 1);
    std::string beforeLeaf = gamePath.substr(0, sl);
    size_t sl2 = beforeLeaf.find_last_of('/');
    parentDir = (sl2 != std::string::npos) ? beforeLeaf.substr(sl2 + 1) : beforeLeaf;
  }
  if (leaf.empty() || gamePath.rfind("/proc/self/fd/", 0) == 0) { leaf = "rucni_vyber"; parentDir.clear(); }
  size_t dot = leaf.find_last_of('.');
  if (dot != std::string::npos && dot > 0) leaf = leaf.substr(0, dot);
  if (!parentDir.empty() && parentDir != "ps1_games" && parentDir != "PS1") leaf = parentDir + "__" + leaf;
  for (size_t i = 0; i < leaf.size(); ++i) { char c = leaf[i]; if (!isalnum((unsigned char)c) && c != '-' && c != '_') leaf[i] = '_'; }
  g_srm_path = g_savedir + "/" + leaf + ".srm";
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
