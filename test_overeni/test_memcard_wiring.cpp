/* test_memcard_wiring.c
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

/* ==== minimalni nahrady typu z libretro.h, hodnoty overene primo v
   deps/libretro-common/include/libretro.h tohohle projektu ==== */
#define RETRO_ENVIRONMENT_GET_VARIABLE 15
#define RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY 31
#define RETRO_MEMORY_SAVE_RAM 0
#define RETRO_MEMORY_SYSTEM_RAM 2
struct retro_variable { const char *key; const char *value; };
typedef bool (*retro_environment_t)(unsigned cmd, void *data);
static retro_environment_t environ_cb;

/* ==== napodobenina appciny casti (nap_ps1_native.cpp) - jen typy a
   konstanty, co load_memcards/retro_get_memory_* potrebuji ==== */
#define ENV_GET_SAVE_DIRECTORY 31
#define ENV_GET_VARIABLE 15

/* ==== napodobenina jadra (sio.h/psxcommon.h) - jen to, co
   load_memcards()/retro_get_memory_* skutecne pouzivaji ==== */
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
static char loadmcd_last_path[256] = "";
void LoadMcd(int mcd, char *str) { loadmcd_calls++; snprintf(loadmcd_last_path, sizeof(loadmcd_last_path), "%s", str ? str : "(null)"); }
void LogErr(const char *fmt, ...) { /* tiche - jen abychom videli vysledek testu, ne sum */ }
void SysPrintf(const char *fmt, ...) { }
void get_dash_serial(char *buf, size_t n) { snprintf(buf, n, "TEST-00000"); }

/* ==== SKUTECNA funkce z jadra, verbatim (viz load_memcards_REAL.c) ==== */
#include "load_memcards_REAL.c"

/* ==== SKUTECNE funkce z jadra, verbatim (viz retro_get_memory_REAL.c) ==== */
#include "retro_get_memory_REAL.c"

/* ==== appcina strana: nap_env, co odpovida na environ_cb - staveny
   presne podle nap_ps1_native.cpp (case ENV_GET_SAVE_DIRECTORY a
   ENV_GET_VARIABLE), jen s NAP_CORE_OPTION_FILE prepnutelnym mezi
   starou/novou verzi pres makro pri prekladu ==== */
static const char *g_savedir_test = "/data/test/ps1_saves";

#ifdef POUZIT_STAROU_VERZI
#include "nap_core_option_value_STARA_NEOPRAVENA.c"
#else
#include "nap_core_option_value_REAL.c"
#endif

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

int main(void) {
#ifdef POUZIT_STAROU_VERZI
  printf("=== TEST SE STAROU (B215, PRED OPRAVOU) nap_core_option_value ===\n");
#else
  printf("=== TEST S NOVOU (B216, OPRAVENOU) nap_core_option_value ===\n");
#endif

  environ_cb = test_nap_env;
  memcard_type[0] = MEMCARDTYPE_NONE; /* explicitni vychozi, jako v jadre */
  memcard_type[1] = MEMCARDTYPE_NONE;

  load_memcards();

  printf("memcard_type[0] = %d (0=NONE 1=SERIAL 2=SHARED 3=LIBRETRO)\n", memcard_type[0]);
  printf("memcard_type[1] = %d\n", memcard_type[1]);
  printf("LoadMcd() zavolano %d-krat, posledni cesta='%s'\n", loadmcd_calls, loadmcd_last_path);

  void *mem = retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
  size_t sz = retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
  printf("retro_get_memory_data(SAVE_RAM) = %s\n", mem ? "NENULOVY (ukazuje na Mcd1Data)" : "NULL");
  printf("retro_get_memory_size(SAVE_RAM) = %zu bajtu (ocekavano 131072 = 128KB pri uspechu)\n", sz);

  int ok = (memcard_type[0] == MEMCARDTYPE_LIBRETRO) && (mem == Mcd1Data) && (sz == MCD_SIZE);
  printf("\nVYSLEDEK: %s\n", ok ? "MEMORY KARTA JE ZAPNUTA A DAVA SKUTECNOU 128KB PAMET" : "MEMORY KARTA JE VYPNUTA (core_da_sram by bylo NE)");
  return ok ? 0 : 1;
}
