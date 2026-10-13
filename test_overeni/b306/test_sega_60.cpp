// B306: SKUTECNE vlakno emulace Segy (nap_real_worker_thread) se SKUTECNYM jadrem
// clownmdemu - ne nahrazka. Misto hry maly program pro 68000 (zapne obraz VDP, H40,
// pak smycka), jadro tedy bezi presne jako s ROM v telefonu: 60 kroku/s, radky VDP,
// FM / PSG na konci kroku, fronta zvuku, "Java AudioTrack" (vlakno, ktere si bere
// zvuk po 1024 ramcich jako MainActivity).
//
// Overuje B306 "Sega na TV 60 snimku/s i na S8":
//   a) rezim S8 (no-starve), nikdo se nediva: displej telefonu 30 snimku/s (jako dosud)
//   b) rezim S8, prohlizec pripojeny: TV 60 snimku/s, displej telefonu dal 30/s
//   c) rezim LOW (zvoleny uzivatelem): TV jen 30/s (respektuje volbu)
//   d) bez S8 (jiny telefon): displej i TV 60/s
//   e) zvuk na TV: 48000 vzorku/s z jadra (nap_audio_push_*), paket = snimek + zvuk tehoz kroku
//   f) zvuk hladovi (reproduktor bere rychleji, nez jadro vyrabi): TV ustoupi zvuku
//
//   g++ -std=c++17 -O2 -pthread <prepinace Segy jako v JAK_SPUSTIT.txt 3b> -o test_sega_60 test_sega_60.cpp <objekty jadra>
#include "../../app/src/main/cpp/nap_sega_native_proof.cpp"
#include <cstdio>
#include <thread>
#include <vector>

struct ANativeWindow { int refs = 1; };
void ANativeWindow_acquire(ANativeWindow *w) { w->refs++; }
void ANativeWindow_release(ANativeWindow *w) { w->refs--; }
int32_t ANativeWindow_getWidth(ANativeWindow *) { return 0; }
int32_t ANativeWindow_getHeight(ANativeWindow *) { return 0; }
int32_t ANativeWindow_getFormat(ANativeWindow *) { return 0; }
int32_t ANativeWindow_setBuffersGeometry(ANativeWindow *, int32_t, int32_t, int32_t) { return 0; }
int32_t ANativeWindow_lock(ANativeWindow *, ANativeWindow_Buffer *, ARect *) { return -1; }
int32_t ANativeWindow_unlockAndPost(ANativeWindow *) { return 0; }
ANativeWindow *ANativeWindow_fromSurface(JNIEnv *, jobject) { return nullptr; }

static int g_ok = 0, g_chyb = 0;
static void kontrola(bool ok, const std::string &co) {
  std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  (ok ? g_ok : g_chyb)++;
}

static std::atomic<bool> g_zvukBezi{false};
static std::atomic<int> g_zvukRychlost{100};      // % rychlosti "AudioTracku" (100 = 48000/s)
static void zvukVlakno() {
  std::vector<jshort> b(2048);
  auto t = std::chrono::steady_clock::now();
  while (g_zvukBezi.load()) {
    nap_audio_pull_stereo(b.data(), 1024);
    t += std::chrono::microseconds(1024 * 1000000LL / 48000 * 100 / std::max(1, g_zvukRychlost.load()));
    std::this_thread::sleep_until(t);
  }
}

struct Mereni { double displej, tvSnimku, pakety, zvuk, hlad; };
static Mereni mer(double s, bool klient) {
  std::atomic<bool> konec{false};
  std::atomic<long long> pak{0}, vz{0};
  std::thread pr;
  int id = 0;
  if (klient) {
    id = g_segaProud.otevri();
    pr = std::thread([&] {             // Java napTvWebJadroProud: proudDalsiNative dokola
      std::vector<uint8_t> pk;
      while (!konec.load()) {
        if (g_segaProud.dalsi(id, 250, 0, pk) == 1 && pk.size() > 32) {
          pak++;
          // zvuk: za obrazem verze 2 (palZmen, LZ)
          const uint8_t *p = pk.data() + 4;
          size_t o = 28;
          const unsigned pz = p[o] | (p[o + 1] << 8);
          o += 2 + 4 * pz;
          const size_t dl = (size_t)p[o] | ((size_t)p[o + 1] << 8) | ((size_t)p[o + 2] << 16) | ((size_t)p[o + 3] << 24);
          o += 4 + dl;
          vz += p[o] | (p[o + 1] << 8);
        }
      }
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(500));   // ustalit
  }
  const unsigned long long d0 = g_render_captured_frames.load(), t0 = g_segaProud.snimkuCelkem();
  const long long p0 = pak.load(), v0 = vz.load();
  const uint64_t h0 = g_audio_starve_events.load();
  std::this_thread::sleep_for(std::chrono::milliseconds((int)(s * 1000)));
  Mereni m;
  m.displej = (g_render_captured_frames.load() - d0) / s;
  m.tvSnimku = (g_segaProud.snimkuCelkem() - t0) / s;
  m.pakety = (pak.load() - p0) / s;
  m.zvuk = (vz.load() - v0) / s;
  m.hlad = (double)(g_audio_starve_events.load() - h0);
  if (klient) {
    konec = true;
    pr.join();
    g_segaProud.zavri(id);
  }
  return m;
}

int main() {
  std::printf("B306: skutecne vlakno emulace Segy + jadro clownmdemu (program pro 68000 misto hry)\n\n");
  // ---- ROM: vektory + kratky program (obraz zapnout, H40, smycka) ----
  std::vector<uint8_t> rom(0x10000, 0);
  auto w32 = [&](size_t a, uint32_t v) { rom[a] = v >> 24; rom[a + 1] = (v >> 16) & 255; rom[a + 2] = (v >> 8) & 255; rom[a + 3] = v & 255; };
  auto w16 = [&](size_t a, uint16_t v) { rom[a] = v >> 8; rom[a + 1] = v & 255; };
  w32(0, 0x00FFFE00);              // SSP
  w32(4, 0x00000200);              // PC
  std::memcpy(&rom[0x100], "SEGA MEGA DRIVE ", 16);
  std::memcpy(&rom[0x1F0], "JUE             ", 16);
  size_t a = 0x200;
  w16(a, 0x46FC); w16(a + 2, 0x2700); a += 4;                                         // move #$2700,sr
  w16(a, 0x33FC); w16(a + 2, 0x8174); w32(a + 4, 0x00C00004); a += 8;                 // VDP reg 1: obraz zap, V28, DMA
  w16(a, 0x33FC); w16(a + 2, 0x8C81); w32(a + 4, 0x00C00004); a += 8;                 // VDP reg 12: H40 (320 bodu)
  w16(a, 0x60FE);                                                                     // bra.s *
  const std::string r = nap_real_load_rom_bytes(rom.data(), rom.size());
  std::printf("%s\n", r.substr(0, r.find('\n')).c_str());
  g_zvukBezi = true;
  std::thread zv(zvukVlakno);
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));

  // ---- a) S8, nikdo se nediva ----
  g_mobile_no_starve.store(1);
  g_native_perf_low.store(0);
  std::this_thread::sleep_for(std::chrono::milliseconds(800));
  Mereni m = mer(4.0, false);
  const double hladA = m.hlad;
  std::printf("a) S8, bez prohlizece: displej %.1f snimku/s, snimku pro prenos %.1f/s, nizka fronta zvuku %.0fx\n", m.displej, m.tvSnimku, m.hlad);
  kontrola(m.displej > 27 && m.displej < 33, "a) rezim S8: displej telefonu 30 snimku/s (jako dosud, B306 ho nemeni)");
  // ---- b) S8 + prohlizec ----
  m = mer(4.0, true);
  std::printf("b) S8 + prohlizec: displej %.1f/s, TV snimku %.1f/s, paketu %.1f/s, zvuk %.0f vzorku/s, nizka fronta zvuku %.0fx\n",
              m.displej, m.tvSnimku, m.pakety, m.zvuk, m.hlad);
  kontrola(m.tvSnimku > 57 && m.tvSnimku < 62 && m.pakety > 55, "b) rezim S8 + prohlizec: TV dostava 60 snimku/s (B305: 30)");
  kontrola(m.displej > 27 && m.displej < 33, "b) displej telefonu pri tom dal 30/s (telefon nema praci navic)");
  kontrola(m.zvuk > 48000 * 0.95 && m.zvuk < 48000 * 1.05, "e) zvuk na TV 48000 vzorku/s primo z jadra (FM + PSG)");
  kontrola(m.hlad <= hladA * 1.5 + 5, "b) fronta zvuku telefonu se s prenosem 60/s nezhorsila (bez prohlizece " + std::to_string((int)hladA) + "x, s nim " + std::to_string((int)m.hlad) + "x)");
  // ---- c) LOW ----
  g_native_perf_low.store(1);
  m = mer(3.0, true);
  std::printf("c) S8 + LOW + prohlizec: displej %.1f/s, TV %.1f/s\n", m.displej, m.tvSnimku);
  kontrola(m.tvSnimku > 27 && m.tvSnimku < 33, "c) zvoleny rezim LOW: TV taky jen 30/s (volba plati)");
  g_native_perf_low.store(0);
  // ---- d) jiny telefon (bez S8) ----
  g_mobile_no_starve.store(0);
  m = mer(3.0, true);
  std::printf("d) bez S8 + prohlizec: displej %.1f/s, TV %.1f/s\n", m.displej, m.tvSnimku);
  kontrola(m.displej > 57 && m.tvSnimku > 57, "d) jiny telefon: displej i TV 60 snimku/s");
  // ---- f) zvuk hladovi: reproduktor bere o 40 % rychleji (jadro dohani - no-starve) ----
  g_mobile_no_starve.store(1);
  g_zvukRychlost.store(140);
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  Mereni f1 = mer(4.0, false);
  Mereni f2 = mer(4.0, true);
  std::printf("f) S8, reproduktor bere o 40 %% vic: bez prohlizece displej %.1f/s nizka fronta %.0fx; s prohlizecem displej %.1f/s, TV %.1f/s, nizka fronta %.0fx\n",
              f1.displej, f1.hlad, f2.displej, f2.tvSnimku, f2.hlad);
  kontrola(f2.hlad <= f1.hlad * 1.3 + 10, "f) kdyz zvuk telefonu hladovi, prenos na TV ho nezhorsi (pri hladu se pro TV nesnima)");
  g_zvukRychlost.store(100);

  g_zvukBezi = false;
  zv.join();
  nap_real_stop_worker();
  std::printf("\n%s\nVYSLEDEK: %d kontrol, %d chyb\n", g_segaProud.vezmiLog().c_str(), g_ok + g_chyb, g_chyb);
  return g_chyb ? 1 : 0;
}
