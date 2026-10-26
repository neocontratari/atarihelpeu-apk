/* B316 test: nahrada rtime.h (Atari800) - hodiny R-Time 8 v testu vypnute */
#ifndef RTIME_H_
#define RTIME_H_
#include "atari.h"
extern int RTIME_enabled;
UBYTE RTIME_GetByte(void);
void RTIME_PutByte(UBYTE byte);
#endif
