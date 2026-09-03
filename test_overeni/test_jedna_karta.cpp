/* test_jedna_karta.cpp
 *
 * Rene: "memory card nepracuje jak na ps1... presne tak ze je porad
 * zasunuta v ps1." Predchozi model (B215-B217) delal SAMOSTATNOU
 * kartu pro kazdou hru - spatne. Tenhle test overuje OPRAVENOU
 * architekturu: JEDNA trvala karta, sdilena mezi VSEMI hrami i BIOSem
 * bez disku, presne jako fyzicky hardware.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <cstdint>
#include <string>

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
static enum { MEMCARDTYPE_NONE=0, MEMCARDTYPE_SERIAL, MEMCARDTYPE_SHARED, MEMCARDTYPE_LIBRETRO } memcard_type[2];
static int loadmcd_calls = 0;
void LoadMcd(int mcd, char *str) { loadmcd_calls++; }
void LogErr(const char *fmt, ...) { }
void SysPrintf(const char *fmt, ...) { }
void get_dash_serial(char *buf, size_t n) { snprintf(buf, n, "TEST-00000"); }
#include "load_memcards_REAL.c"
#include "retro_get_memory_REAL.c"
static const char *g_savedir_test = "/tmp/nap_jedna_karta_test";
#include "nap_core_option_value_REAL.c"
static bool test_nap_env(unsigned cmd, void *data) {
  switch (cmd) {
    case ENV_GET_SAVE_DIRECTORY: { *(const char**)data = g_savedir_test; return true; }
    case ENV_GET_VARIABLE: {
      struct retro_variable *var = (struct retro_variable*)data;
      const char *value = var ? nap_core_option_value(var->key) : NULL;
      if (!value) { if (var) var->value = NULL; return false; }
      var->value = value; return true;
    }
    default: return false;
  }
}
#define NAPLOG(...) fprintf(stderr, "[NAPLOG] " __VA_ARGS__), fprintf(stderr, "\n")
static std::string g_savedir = "/tmp/nap_jedna_karta_test";
static std::string g_srm_path;
static int g_srm_last_save_ok = -1;
static size_t g_srm_last_size = 0;
static uint32_t g_srm_last_fnv = 0;
static uint32_t nap_fnv32(const uint8_t *d, size_t n) { uint32_t h=2166136261u; for(size_t i=0;i<n;++i){h^=d[i];h*=16777619u;} return h; }
#include "nap_srm_functions_REAL.cpp"

static void boot(const char *gamePath, const char *popis) {
  printf("\n--- BOOT: %s ---\n", popis);
  memcard_type[0]=MEMCARDTYPE_NONE; memcard_type[1]=MEMCARDTYPE_NONE;
  load_memcards();
  nap_srm_set_path(gamePath ? gamePath : "");
  nap_srm_load();
  printf("  g_srm_path = %s\n", g_srm_path.c_str());
}

int main(void) {
  system("rm -rf /tmp/nap_jedna_karta_test && mkdir -p /tmp/nap_jedna_karta_test");
  environ_cb = test_nap_env;

  printf("=== KROK 1: BIOS bez disku (prvni zapnuti appky, nikdy nic nehrano) ===\n");
  boot(nullptr, "BIOS bez disku");
  std::string cesta_bios = g_srm_path;
  bool bios_prazdny_pri_prvnim_zapnuti = true;
  for (int i = 0; i < 16; i++) if (Mcd1Data[i] != 0) { bios_prazdny_pri_prvnim_zapnuti = false; break; }
  printf("  karta prazdna pri UPLNE prvnim zapnuti appky? %s (spravne)\n", bios_prazdny_pri_prvnim_zapnuti ? "ANO" : "NE");

  printf("\n=== KROK 2: hra A si ulozi pozici, appka se 'vypne' (ps1Stop by tu zavolal save_if_dirty) ===\n");
  boot("/data/ps1_games/gdrive_hra_A/game.bin", "hra A (Crash Bandicoot)");
  std::string cesta_hra_A = g_srm_path;
  strcpy(Mcd1Data, "ULOZENA POZICE - Crash Bandicoot, level 5");
  nap_srm_save_if_dirty("hrac_ukoncil_hru_A");

  printf("\n=== KROK 3: cesta BIOSu a cesta hry A MUSI byt STEJNA (jedna karta, ne dve) ===\n");
  printf("  cesta BIOS:  %s\n", cesta_bios.c_str());
  printf("  cesta hra A: %s\n", cesta_hra_A.c_str());
  printf("  STEJNA cesta? %s (spravne, kdyz ANO - realna PS1 ma jednu kartu)\n", (cesta_bios == cesta_hra_A) ? "ANO" : "NE");

  printf("\n=== KROK 4: 'appka se restartuje' (novy proces - nova instance Mcd1Data v pameti) ===\n");
  memset(Mcd1Data, 0, MCD_SIZE); // simulace: cerstvy proces po restartu appky ma prazdnou pamet
  boot(nullptr, "BIOS bez disku, PO 'restartu appky'");
  bool bios_vidi_ulozenou_hru_po_restartu = (strncmp(Mcd1Data, "ULOZENA POZICE", 14) == 0);
  printf("  Mcd1Data = \"%.50s\"\n", Mcd1Data);
  printf("  BIOS po restartu appky VIDI ulozenou hru z disku? %s (musi byt ANO)\n",
         bios_vidi_ulozenou_hru_po_restartu ? "ANO" : "NE - CHYBA!");

  printf("\n=== KROK 5: hra B (jina hra) nabootovana - MUSI videt tu samou kartu, ne prazdnou novou ===\n");
  boot("/data/ps1_games/url_hra_B/disc1.bin", "hra B (Tekken 3)");
  std::string cesta_hra_B = g_srm_path;
  bool hra_B_vidi_stejnou_kartu = (strncmp(Mcd1Data, "ULOZENA POZICE", 14) == 0);
  printf("  cesta hry B: %s\n", cesta_hra_B.c_str());
  printf("  stejna cesta jako hra A/BIOS? %s\n", (cesta_hra_B == cesta_hra_A) ? "ANO (spravne)" : "NE - CHYBA!");
  printf("  hra B pri nabootovani VIDI ulozenou pozici hry A na karte (koexistuji)? %s\n",
         hra_B_vidi_stejnou_kartu ? "ANO (spravne, presne jako na realne PS1)" : "NE - CHYBA!");

  int ok = bios_prazdny_pri_prvnim_zapnuti
        && (cesta_bios == cesta_hra_A)
        && bios_vidi_ulozenou_hru_po_restartu
        && (cesta_hra_B == cesta_hra_A)
        && hra_B_vidi_stejnou_kartu;
  printf("\nCELKOVY VYSLEDEK: %s\n", ok
    ? "JEDNA TRVALA KARTA, SDILENA BIOSEM I VSEMI HRAMI - presne jako na realne PS1"
    : "NEKTERY KROK SELHAL");
  return ok ? 0 : 1;
}
