/* B316 test: nahrada config.h emulatoru Atari800 - jen aby se dal prelozit
   jejich src/cartridge.c (stazeny v case testu) a porovnat s nasi cartridge.
   BASIC = bez ukladani stavu (statesav.h) - test ho nepotrebuje. */
#ifndef CONFIG_H_
#define CONFIG_H_
#define BASIC 1
#endif
