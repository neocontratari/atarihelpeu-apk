// B323: samotest jadra (nap_atari_selftest.h) - cisla, ktera ma telefon dat (NativeAtariCoreBridge.OCEKAVANY_*).
#include <cstdio>
#include "nap_atari_selftest.h"
int main() {
  long long instr = 0, reads = 0;
  const uint32_t hc = nap::selftest::cpuHash(200, 7u, &instr);
  const uint32_t hm = nap::selftest::memHash(&reads);
  std::printf("cpuHash %08X cpuInstr %lld memHash %08X memReads %lld\n", hc, instr, hm, reads);
  return 0;
}
