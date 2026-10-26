/* B316 test: stav "pocitace" pro src/cartridge.c z Atari800 (130XE, BASIC
   vestaveny). Okna cartridge = priznaky REF_cart809f / REF_cartA0bf,
   obsah oken = MEMORY_mem. Tenhle soubor je nas (zadny kod Atari800). */
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "atari.h"
#include "memory.h"
#include "binload.h"
#include "rtime.h"
#include "util.h"
#include "log.h"

UBYTE MEMORY_mem[65536 + 2];
UBYTE MEMORY_basic[8192];
int MEMORY_ram_size = 64;
int MEMORY_have_basic = 1;
int REF_cart809f = 0, REF_cartA0bf = 0;
int Atari800_machine_type = Atari800_MACHINE_XLXE;
int Atari800_builtin_basic = 1;
int Atari800_disable_basic = 0;
int BINLOAD_loading_basic = 0;
int RTIME_enabled = 0;
int REF_coldstartu = 0, REF_zmen_stroje = 0;

void MEMORY_CopyFromCart(UWORD from, UWORD to, const UBYTE *src) { memcpy(MEMORY_mem + from, src, (size_t)(to - from + 1)); }
void MEMORY_CopyToCart(UWORD from, UWORD to, UBYTE *dst) { memcpy(dst, MEMORY_mem + from, (size_t)(to - from + 1)); }
void MEMORY_dFillMem(UWORD addr, UBYTE value, int length) { memset(MEMORY_mem + addr, value, (size_t)length); }
void MEMORY_Cart809fEnable(void) { REF_cart809f = 1; }
void MEMORY_Cart809fDisable(void) { REF_cart809f = 0; }
void MEMORY_CartA0bfEnable(void) { REF_cartA0bf = 1; }
void MEMORY_CartA0bfDisable(void) { REF_cartA0bf = 0; }
void MEMORY_SetROM(UWORD from, UWORD to) { (void)from; (void)to; }
void MEMORY_SetRAM(UWORD from, UWORD to) { (void)from; (void)to; }
void MEMORY_SetHARDWARE(UWORD from, UWORD to) { (void)from; (void)to; }
void Atari800_SetMachineType(int type) { Atari800_machine_type = type; REF_zmen_stroje++; }
void Atari800_InitialiseMachine(void) {}
void Atari800_Coldstart(void) { REF_coldstartu++; }
UBYTE RTIME_GetByte(void) { return 0; }
void RTIME_PutByte(UBYTE byte) { (void)byte; }
void *Util_malloc(size_t size) { void *p = malloc(size ? size : 1); if (!p) abort(); return p; }
int Util_flen(FILE *fp) { long p = ftell(fp); fseek(fp, 0, SEEK_END); long n = ftell(fp); fseek(fp, p, SEEK_SET); return (int)n; }
size_t Util_strlcpy(char *dest, const char *src, size_t size) {
  size_t n = strlen(src);
  if (size) { size_t k = n < size - 1 ? n : size - 1; memcpy(dest, src, k); dest[k] = 0; }
  return n;
}
int Util_sscandec(const char *s) { return atoi(s); }
int Util_sscansdec(const char *s, int *result) { *result = atoi(s); return 1; }
int Util_sscanbool(const char *s) { return atoi(s) ? 1 : 0; }
void Log_print(const char *format, ...) { (void)format; }
