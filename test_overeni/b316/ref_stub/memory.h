/* B316 test: nahrada memory.h (Atari800). Pamet je obycejne pole 64 kB,
   test si pamatuje, jestli cartridge drzi okno $8000-$9FFF (RD4) a
   $A000-$BFFF (RD5) - to a obsah oken pak porovnava s nasi cartridge. */
#ifndef MEMORY_H_
#define MEMORY_H_
#include "atari.h"
#include <string.h>
extern UBYTE MEMORY_mem[65536 + 2];
extern UBYTE MEMORY_basic[8192];
extern int MEMORY_ram_size;
extern int MEMORY_have_basic;
extern int REF_cart809f, REF_cartA0bf;          /* okna cartridge zapnuta (RD4 / RD5) */
#define MEMORY_dGetByte(x) (MEMORY_mem[x])
void MEMORY_CopyFromCart(UWORD from, UWORD to, const UBYTE *src);
void MEMORY_CopyToCart(UWORD from, UWORD to, UBYTE *dst);
void MEMORY_dFillMem(UWORD addr, UBYTE value, int length);
void MEMORY_Cart809fEnable(void);
void MEMORY_Cart809fDisable(void);
void MEMORY_CartA0bfEnable(void);
void MEMORY_CartA0bfDisable(void);
void MEMORY_SetROM(UWORD from, UWORD to);
void MEMORY_SetRAM(UWORD from, UWORD to);
void MEMORY_SetHARDWARE(UWORD from, UWORD to);
#endif
