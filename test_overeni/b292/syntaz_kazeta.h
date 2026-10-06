// syntaz_kazeta.h - B298: synteticka kazeta Atari (WAV) pro testy.
// FSK jako skutecny magnetofon: 5327 Hz = 1 (mark), 3995 Hz = 0 (space),
// 600 baudu, 20 s uvodniho tonu, zaznamy 55 55 ctl + 128 dat + soucet.
// Bootovaci "hra": 1 zaznam na $0700, init $070C nastavi DOSVEC na $0715,
// tam COLOR4 = $34, $0600 = $42 a nekonecna smycka (JMP $071F).
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

inline std::vector<uint8_t> syntazBootKazetaWav(bool stereo = false) {
  const int SR = 44100;
  uint8_t code[128] = {0};
  const uint8_t prog[] = {0x00, 0x01, 0x00, 0x07, 0x0C, 0x07,          // flag, 1 zaznam, $0700, init $070C
                          0x18, 0x60,                                  // $0706: CLC RTS = boot OK
                          0xEA, 0xEA, 0xEA, 0xEA,
                          0xA9, 0x15, 0x85, 0x0A, 0xA9, 0x07, 0x85, 0x0B, 0x60,   // DOSVEC = $0715
                          0xA9, 0x34, 0x8D, 0xC8, 0x02,                // COLOR4 = $34
                          0xA9, 0x42, 0x8D, 0x00, 0x06,                // $0600 = $42
                          0x4C, 0x1F, 0x07};                           // JMP $071F
  std::memcpy(code, prog, sizeof(prog));
  std::vector<float> s;
  double faze = 0;
  auto ton = [&](double f, double sek) {
    const int n = (int)std::lround(sek * SR);
    for (int i = 0; i < n; i++) { s.push_back((float)std::sin(faze)); faze += 2 * 3.14159265358979323846 * f / SR; }
  };
  auto zaznam = [&](uint8_t ctl, const uint8_t *d) {
    std::vector<uint8_t> r = {0x55, 0x55, ctl};
    r.insert(r.end(), d, d + 128);
    unsigned c = 0;
    for (uint8_t b : r) { c += b; if (c > 255) c = (c & 0xFF) + 1; }
    r.push_back((uint8_t)c);
    for (uint8_t b : r) {
      int bity[10] = {0};
      for (int k = 0; k < 8; k++) bity[k + 1] = (b >> k) & 1;
      bity[9] = 1;
      for (int v : bity) ton(v ? 5327 : 3995, 1.0 / 600);
    }
  };
  uint8_t nuly[128] = {0};
  ton(5327, 20.0);
  zaznam(0xFC, code);
  ton(5327, 3.0);
  zaznam(0xFE, nuly);
  ton(5327, 1.0);
  const int nch = stereo ? 2 : 1;
  std::vector<uint8_t> w;
  auto u32 = [&](uint32_t v) { for (int k = 0; k < 4; k++) w.push_back((uint8_t)(v >> (8 * k))); };
  auto u16 = [&](uint16_t v) { w.push_back((uint8_t)v); w.push_back((uint8_t)(v >> 8)); };
  const uint32_t dataLen = (uint32_t)(s.size() * 2 * nch);
  w.insert(w.end(), {'R', 'I', 'F', 'F'}); u32(36 + dataLen); w.insert(w.end(), {'W', 'A', 'V', 'E'});
  w.insert(w.end(), {'f', 'm', 't', ' '}); u32(16); u16(1); u16((uint16_t)nch); u32(SR); u32((uint32_t)(SR * 2 * nch)); u16((uint16_t)(2 * nch)); u16(16);
  w.insert(w.end(), {'d', 'a', 't', 'a'}); u32(dataLen);
  for (size_t i = 0; i < s.size(); i++) {
    if (stereo) {   // levy kanal = zvukova stopa (440 Hz), pravy = data
      const int16_t a = (int16_t)std::lround(0.3 * 32767 * std::sin(2 * 3.14159265358979323846 * 440 * (double)i / SR));
      u16((uint16_t)a);
    }
    u16((uint16_t)(int16_t)std::lround(s[i] * 0.7 * 32767));
  }
  return w;
}
