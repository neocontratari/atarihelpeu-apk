// nap_tv_proud_spol.h
// B305: spolecne drobnosti prenosu Sega / PS1 na TV / PC (zapis cisel do
// paketu, porovnani useku, cas). Bez Androidu.
#pragma once
#include <chrono>
#include <cstdint>
#include <cstring>
#include <vector>
#if defined(__linux__) || defined(__ANDROID__)
#include <sys/resource.h>
#endif

namespace nap {
namespace proud {

// B306: vlakno prenosu (pasy JPEG) o kousek vys nez bezna vlakna - jako Android
// THREAD_PRIORITY_DISPLAY (-4). Na telefonu se tak kodovani neztrati mezi ostatnimi
// vlakny appky; jadro emulace (-8) i zvuk (-19) maji porad prednost. Na Linuxu /
// Androidu plati setpriority(PRIO_PROCESS, 0) jen pro volajici vlakno. Kdyz to
// system nedovoli, nic se nedeje. Vraci vysledek (0 = ok) pro log / test.
inline int zvysPrioritu(int nice = -4) {
#if defined(__linux__) || defined(__ANDROID__)
  return setpriority(PRIO_PROCESS, 0, nice);
#else
  (void)nice;
  return -1;
#endif
}

inline long long tedMs() {
  return (long long)std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline double tedMsPresne() {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

inline void p8(std::vector<uint8_t> &o, unsigned v) { o.push_back((uint8_t)v); }
inline void p16(std::vector<uint8_t> &o, unsigned v) { o.push_back((uint8_t)v); o.push_back((uint8_t)(v >> 8)); }
inline void p32(std::vector<uint8_t> &o, uint32_t v) { p16(o, v & 0xFFFFu); p16(o, v >> 16); }
inline void pVarint(std::vector<uint8_t> &o, size_t v) {
  while (v >= 0x80) { o.push_back((uint8_t)(0x80 | (v & 0x7F))); v >>= 7; }
  o.push_back((uint8_t)v);
}
inline size_t varintDelka(size_t v) { size_t n = 1; while (v >= 0x80) { v >>= 7; n++; } return n; }

// delka shody dvou useku (az max), po 8 bajtech
inline size_t shoda(const uint8_t *a, const uint8_t *b, size_t max) {
  size_t n = 0;
  while (n + 8 <= max) {
    uint64_t x, y;
    std::memcpy(&x, a + n, 8); std::memcpy(&y, b + n, 8);
    const uint64_t d = x ^ y;
    if (d) return n + ((size_t)__builtin_ctzll(d) >> 3);
    n += 8;
  }
  while (n < max && a[n] == b[n]) n++;
  return n;
}

}  // namespace proud
}  // namespace nap
