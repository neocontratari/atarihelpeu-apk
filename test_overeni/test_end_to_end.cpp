/* test_end_to_end.cpp
 *
 * *** ZASTARALE OD B218 - NESPOUSTET SAMOSTATNE BEZ ROZMYSLU ***
 * Tenhle test overoval architekturu "kazda hra ma svou vlastni kartu",
 * o ktere se v B218 zjistilo, ze je SPATNE - realna PS1 ma jednu
 * trvalou kartu sdilenou vsemi hrami (viz test_jedna_karta.cpp a
 * CO_JE_V_B218.md). Kroky 2-4 tohohle testu (ocekavaji RUZNE cesty
 * pro ruzne hry) uz se SKUTECNYM zdrojovym kodem od B218 NEPROJDOU -
 * a je to spravne, dokazuji tim presne tu zmenu chovani. Necham ho tu
 * jako zaznam, jak vypadala predchozi (chybna) uvaha, ne jako aktualni
 * test spravnosti - tim je ted test_jedna_karta.cpp.
 *
 * CIL: overit, ze SKUTECNA funkce load_memcards() z PCSX ReARMed
 * (vytazena verbatim z vendor/pcsx_rearmed/frontend/libretro.c) skutecne
 * zapne memcard_type[0]=MEMCARDTYPE_LIBRETRO, kdyz se jako environ_cb
 * pouzije appce SKUTECNA nap_env()/nap_core_option_value() (vytazena
 * verbatim z nap_ps1_native.cpp) - a ze retro_get_memory_data/size
 * (taky verbatim z jadra) v dusledku toho vrati platnou pamet.
 *
 * Spousti se DVAKRAT - jednou se STAROU (B215, chybi radky memcard1/2)
 * verzi nap_core_option_value a jednou s NOVOU (B216, opravenou).
 * Ocekavany vysledek: stara verze -> MEMCARDTYPE_NONE, NULL, 0.
 *                      nova verze -> MEMCARDTYPE_LIBRETRO, ne-NULL, 128KB.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <cstdint>
#include <string>
#include <cctype>
#include <mutex>

#define RETRO_ENVIRONMENT_GET_VARIABLE 15
#define RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY 31
#define RETRO_MEMORY_SAVE_RAM 0
#define RETRO_MEMORY_SYSTEM_RAM 2
struct retro_variable { const char *key; const char *value; };
typedef bool (*retro_environment_t)(unsigned cmd, void *data);
static retro_environment_t environ_cb;

#define ENV_GET_SAVE_DIRECTORY 31
#define ENV_GET_VARIABLE 15

#define MCD_SIZE (1024 * 8 * 16)
char Mcd1Data[MCD_SIZE], Mcd2Data[MCD_SIZE];
struct { char Mcd1[256]; char Mcd2[256]; } Config;
char CdromId[16] = "";
struct { struct { void *psxM; } ptrs; } psxRegs;
static enum {
   MEMCARDTYPE_NONE = 0,
   MEMCARDTYPE_SERIAL,
   MEMCARDTYPE_SHARED,
   MEMCARDTYPE_LIBRETRO,
} memcard_type[2];

static int loadmcd_calls = 0;
void LoadMcd(int mcd, char *str) { loadmcd_calls++; }
void LogErr(const char *fmt, ...) { }
void SysPrintf(const char *fmt, ...) { }
void get_dash_serial(char *buf, size_t n) { snprintf(buf, n, "TEST-00000"); }

#include "load_memcards_REAL.c"
#include "retro_get_memory_REAL.c"

static const char *g_savedir_test = "/tmp/nap_e2e_test_savedir";
#include "nap_core_option_value_REAL.c"

static bool test_nap_env(unsigned cmd, void *data) {
  switch (cmd) {
    case ENV_GET_SAVE_DIRECTORY: { *(const char**)data = g_savedir_test; return true; }
    case ENV_GET_VARIABLE: {
      struct retro_variable *var = (struct retro_variable*)data;
      const char *value = var ? nap_core_option_value(var->key) : NULL;
      if (!value) { if (var) var->value = NULL; return false; }
      var->value = value;
      return true;
    }
    default: return false;
  }
}

/* ==== appcina skutecna nap_srm_* logika (B215+B216), vytazena
   doslovne - potrebuje NAPLOG, g_savedir a par typu z nap_ps1_native.cpp ==== */
#define NAPLOG(...) fprintf(stderr, "[NAPLOG] " __VA_ARGS__), fprintf(stderr, "\n")
static std::string g_savedir = "/tmp/nap_e2e_test_savedir";
static std::string g_srm_path;
static int g_srm_last_save_ok = -1;
static size_t g_srm_last_size = 0;
static uint32_t g_srm_last_fnv = 0;
static uint32_t nap_fnv32(const uint8_t *d, size_t n) {
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < n; ++i) { h ^= d[i]; h *= 16777619u; }
  return h;
}
#include "nap_srm_functions_REAL.cpp"

static void simulate_boot(const char *gameFullPath, const char *label) {
  printf("\n--- BOOT: %s (label=%s) ---\n", gameFullPath, label);
  /* presne poradi jako ve skutecnem kodu: retro_load_game (= memcard_type
     se nastavi UVNITR load_memcards, ktere jadro vola behem load_game)
     PRED nap_srm_set_path/nap_srm_load. Tady load_memcards volame primo,
     protoze plny retro_load_game potrebuje cely CD image parser - ale
     poradi vuci srm funkcim je STEJNE, jen bez zbytku jadra okolo. */
  memcard_type[0] = MEMCARDTYPE_NONE; memcard_type[1] = MEMCARDTYPE_NONE;
  load_memcards();
  nap_srm_set_path(gameFullPath);
  nap_srm_load();
  printf("  g_srm_path = %s\n", g_srm_path.c_str());
}

int main(void) {
  system("rm -rf /tmp/nap_e2e_test_savedir && mkdir -p /tmp/nap_e2e_test_savedir");
  environ_cb = test_nap_env;

  printf("=== KROK 1: dve RUZNE hry se STEJNYM nazvem souboru (bezny pripad PS1 dumpu) ===\n");
  simulate_boot("/data/ps1_games/gdrive_hra_A/game.bin", "game.bin");
  std::string cesta_A = g_srm_path;

  /* hra A si neco "ulozi" - simulace zapisu do jejiho SAVE_RAM */
  memset(Mcd1Data, 0, MCD_SIZE);
  strcpy(Mcd1Data, "ULOZENA POZICE HRY A - level 5, 99 zivotu");
  nap_srm_save_if_dirty("test_hra_A_ulozila");

  simulate_boot("/data/ps1_games/url_hra_B_9f8e7d/game.bin", "game.bin");
  std::string cesta_B = g_srm_path;

  printf("\n=== KROK 2: cesty se NESMI shodovat (jinak by se hry prepsaly) ===\n");
  printf("  cesta hry A: %s\n", cesta_A.c_str());
  printf("  cesta hry B: %s\n", cesta_B.c_str());
  printf("  RUZNE cesty? %s\n", (cesta_A != cesta_B) ? "ANO (spravne)" : "NE - KOLIZE!");

  /* hra B "nacte" svoji kartu - MUSI byt prazdna (nova hra), NE obsah hry A */
  printf("\n=== KROK 3: hra B po nabootovani NESMI videt data hry A ===\n");
  bool videt_data_hry_A = (strncmp(Mcd1Data, "ULOZENA POZICE HRY A", 20) == 0);
  printf("  Mcd1Data po nabootovani hry B obsahuje retezec hry A? %s\n",
         videt_data_hry_A ? "ANO - CHYBA, KOLIZE!" : "NE (spravne, cista karta)");

  /* hra B si ulozi svoje vlastni data, ruzna od hry A */
  memset(Mcd1Data, 0, MCD_SIZE);
  strcpy(Mcd1Data, "ULOZENA POZICE HRY B - level 1, 3 zivoty");
  nap_srm_save_if_dirty("test_hra_B_ulozila");

  printf("\n=== KROK 4: znovunabootovani hry A MUSI vratit JEJI data, ne data hry B ===\n");
  simulate_boot("/data/ps1_games/gdrive_hra_A/game.bin", "game.bin");
  printf("  Mcd1Data po znovunabootovani hry A = \"%s\"\n", Mcd1Data);
  bool spravna_data_A = (strncmp(Mcd1Data, "ULOZENA POZICE HRY A", 20) == 0);
  printf("  je to spravne (data hry A, ne hry B)? %s\n", spravna_data_A ? "ANO" : "NE - CHYBA!");

  printf("\n=== KROK 5: soubory na disku - musí existovat DVA ruzne .srm soubory ===\n");
  system("ls -la /tmp/nap_e2e_test_savedir/ 2>&1 | grep srm");

  int ok = (cesta_A != cesta_B) && !videt_data_hry_A && spravna_data_A;
  printf("\nCELKOVY VYSLEDEK: %s\n", ok
    ? "VSECHNY TRI KROKY PROSLY - kolize vyresena, karta zapnuta, dve hry se nemichaji"
    : "NEKTERY KROK SELHAL");
  return ok ? 0 : 1;
}
