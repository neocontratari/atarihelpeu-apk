/* B316 test: nahrada atari.h (Atari800) - typy a stav stroje pro cartridge.c */
#ifndef ATARI_H_
#define ATARI_H_
#include <stdio.h>
typedef unsigned char UBYTE;
typedef signed char SBYTE;
typedef unsigned short UWORD;
typedef signed short SWORD;
typedef unsigned int ULONG;
typedef signed int SLONG;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
enum { Atari800_MACHINE_800, Atari800_MACHINE_XLXE, Atari800_MACHINE_5200 };
extern int Atari800_machine_type;     /* test: XL/XE (130XE) */
extern int Atari800_builtin_basic;    /* test: 1 (BASIC v pocitaci jako 130XE) */
extern int Atari800_disable_basic;
void Atari800_SetMachineType(int type);
void Atari800_InitialiseMachine(void);
void Atari800_Coldstart(void);
#endif
