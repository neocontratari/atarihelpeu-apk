// nap_tv_stranka.h
// B333: STRANKY APPKY NA TV / PC CELE Z C++ - BEZ JAVY (Rene: "tady budu nekompromisni - vypni Javu").
//
// Hry (PS1, Sega, Atari), uvodni demo a hlavni menu jdou na TV proudem z C++ uz od B303-B311.
// Stranky (OPTIONS, napoveda, seznam her PS1, prehravac MP3, YouTube, web ...) ale sly starou
// cestou: Java fotila celou obrazovku (1440x2672) na hlavnim vlakne, kodovala ji do H.264
// (115 Mbit/s) nebo JPEG a prohlizec si pro H.264 stahoval prehravac z internetu. Log B332:
// v OPTIONS 896 z 900 snimku uplne stejnych, kopirovani obrazu ~7 ms na snimek na hlavnim
// vlakne, "enkoder nestiha". Od B333:
//  - Java stranku jen vyfoti (PixelCopy na vlastnim vlakne, zmensene na nejvys 1080 bodu -
//    vic TV neukaze), kdyz se na ni neco zmenilo (a 2x za sekundu i tak) - NapTvStranka.java,
//  - tady se z toho stane stejny proud jako u menu: JPEG jen zmenenych casti, stojici casti
//    se zjemni (text ostry), zvuk v tomtez paketu (prehravac MP3 / YouTube) - /stranka.stream,
//  - prohlizec na TV ho prehraje stejnym prehravacem jako menu a hry (zadne H.264, zadny
//    internet). Stara cesta pres Javu (H.264, MJPEG, JPEG, /audio.raw) je vypnuta.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <vector>
#include "nap_tv_proud.h"
#include "nap_tv_proud_jpeg.h"

namespace nap {
namespace proud {

class Stranka {
 public:
  static constexpr int VZORKOVANI = 44100;   // zvuk v proudu (stereo int16) - jako menu

  // bez orezu cernych okraju (stranka je cela), kodek jako menu (vic obdelniku, zjemneni textu),
  // pomer stran v paketu z rozmeru snimku (na vysku 97:180, na sirku 255:137 ...)
  Stranka() : hub_("STRANKA", VZORKOVANI, 9, 16, false) {
    hub_.nastavKodek(nastavKodekMenu);
    hub_.pomerZRozmeru(true);
  }

  Hub<KodekJpeg> &hub() { return hub_; }

  // pomer stran, ktery pujde v paketu (nap_tv_proud.h pomerStran) - pro testy a log
  static void pomer(int w, int h, int &px, int &py) { pomerStran(w, h, px, py); }

  // snimek stranky z Javy: Bitmap ARGB_8888 = v pameti bajty R G B A, radky shora (radek = stride bajtu)
  void snimekRgba(const uint8_t *rgba, int w, int h, int stride) {
    if (!rgba || w <= 0 || h <= 0 || stride < w * 4) return;
    if (hub_.klientu() <= 0) { hub_.snimek(nullptr, 0, 0); return; }   // jen "stranka bezi"
    {
      std::lock_guard<std::mutex> l(m_);
      const size_t n = (size_t)w * h;
      if (argb_.size() != n) argb_.resize(n);
      for (int y = 0; y < h; y++) {
        const uint8_t *s = rgba + (size_t)y * stride;
        uint32_t *d = &argb_[(size_t)y * w];
        for (int x = 0; x < w; x++, s += 4) d[x] = ((uint32_t)s[0] << 16) | ((uint32_t)s[1] << 8) | (uint32_t)s[2];
      }
      hub_.snimek(argb_.data(), w, h);
    }
  }

  // zvuk stranky (prehravac MP3, YouTube): PCM 16 bit little endian, 1 nebo 2 kanaly, libovolna
  // frekvence -> stereo 44100 Hz (linearni prevzorkovani, navazuje mezi kusy)
  void zvuk(const uint8_t *pcm, size_t bajtu, int kanalu, int frekvence) {
    if (!pcm || kanalu < 1 || kanalu > 2 || frekvence < 8000 || frekvence > 192000) return;
    const size_t ramcu = bajtu / (size_t)(2 * kanalu);
    if (ramcu == 0 || hub_.klientu() <= 0) return;
    std::lock_guard<std::mutex> l(zm_);
    ven_.clear();
    auto vzorek = [&](size_t i, int k) -> int16_t {
      const uint8_t *p = pcm + (i * (size_t)kanalu + (size_t)(kanalu == 2 ? k : 0)) * 2;
      return (int16_t)(uint16_t)(p[0] | (p[1] << 8));
    };
    if (frekvence == VZORKOVANI) {
      ven_.reserve(ramcu * 2);
      for (size_t i = 0; i < ramcu; i++) { ven_.push_back(vzorek(i, 0)); ven_.push_back(vzorek(i, 1)); }
    } else {
      // poloha mezi minulym vzorkem (index -1 = posledni z minuleho kusu) a dalsimi
      const double krok = (double)frekvence / VZORKOVANI;
      while (true) {
        const double p = faze_;
        const long i0 = (long)std::floor(p);
        if (i0 + 1 >= (long)ramcu) break;
        const double t = p - (double)i0;
        for (int k = 0; k < 2; k++) {
          const double a = i0 < 0 ? (double)minuly_[k] : (double)vzorek((size_t)i0, k);
          const double b = (double)vzorek((size_t)(i0 + 1), k);
          ven_.push_back((int16_t)std::lround(a + (b - a) * t));
        }
        faze_ += krok;
      }
      faze_ -= (double)ramcu;                    // dalsi kus: index posunuty o tenhle kus
      minuly_[0] = vzorek(ramcu - 1, 0);
      minuly_[1] = vzorek(ramcu - 1, 1);
    }
    if (!ven_.empty()) hub_.zvuk(ven_.data(), ven_.size());
  }

 private:
  Hub<KodekJpeg> hub_;
  std::mutex m_, zm_;
  std::vector<uint32_t> argb_;
  std::vector<int16_t> ven_;
  double faze_ = -1.0;                           // prvni kus: zacina na "minulem" vzorku (ticho)
  int16_t minuly_[2] = {0, 0};
};

}  // namespace proud
}  // namespace nap
