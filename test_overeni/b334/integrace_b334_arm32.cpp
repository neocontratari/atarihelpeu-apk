// B334: SKUTECNE jadro PS1 z appky na 32bitovem ARM (armeabi-v7a - Android TV): dynarec ari64 pro ARM,
// GTE v assembleru ARM / NEON, vykreslovac gpu_neon v assembleru NEON - spousteno v qemu-arm.
// Stejny program kresleni_test.exe (B332: kresli jako hra - retez paketu pres DMA, 2x, cislo snimku
// v obraze, cteni VRAM zpet) a stejny kod appky (nap_ps1_native.cpp + JNI jako z Javy) jako B332.
//  A) jadro bezi na ARM32 s DYNARECEM (psxCpu = psxRec), kresleni ve vlakne (gpu_async) i bez nej,
//  O) obraz ve vlakne = bez vlakna (bajt po bajtu) a cteni VRAM stejne,
//  R) OBRAZ STEJNY JAKO NA PC: kazdy snimek (podle cisla v obraze) bajt po bajtu stejny jako referencni
//     beh na PC (x86: interpret procesoru + gpu_neon pres SSE) - ARM dynarec = interpret, NEON
//     assembler = SIMD C; stejne soucty cteni VRAM,
//  S) ulozeni a nahrani pozice na ARM32.
// Pouziti:  integrace_b334 PRAC kresleni_test.exe zapis REF.txt     (PC: zapise referenci)
//           integrace_b334 PRAC kresleni_test.exe srovnej REF.txt   (ARM32 v qemu-arm: porovna)
// Preklad: viz JAK_SPUSTIT.txt (B) - na PC s objekty ../b332/stavba_jadra_gpu.sh, pro ARM32
// s objekty stavba_jadra_arm32.sh (arm-linux-gnueabihf-g++) a nahradami EGL / GLES (nahrady_gl.c).
#define main integrace_b332_main
#include "../b332/integrace_b332_jadro.cpp"
#undef main

extern "C" {
struct R3000Acpu;
extern struct R3000Acpu *psxCpu;
extern struct R3000Acpu psxInt;
}

static const char *architektura() {
#if defined(__arm__)
  return "ARM 32 bitu (armeabi-v7a)";
#elif defined(__aarch64__)
  return "ARM 64 bitu";
#elif defined(__x86_64__)
  return "PC x86_64";
#else
  return "jina";
#endif
}

int main(int argc, char **argv) {
  if (argc < 5) { std::fprintf(stderr, "pouziti: integrace_b334 PRAC kresleni_test.exe zapis|srovnej REF.txt\n"); return 2; }
  const std::string prac = argv[1], exe = argv[2], rezim = argv[3], ref = argv[4];
  pripravEnv();
  std::printf("B334 - SKUTECNE jadro PS1 z appky na: %s (kresleni_test.exe, BIOS HLE, 2x, kresleni ve vlakne)\n", architektura());
  const std::string sys = prac + "/system", karta = prac + "/karta_b334";
  mkdir(sys.c_str(), 0755);
  mkdir(karta.c_str(), 0755);
  FakeStr diag{prac + "/nap_tv_session_log_b334.txt"};
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetDiagLogPath(&g_env, nullptr, (jstring)&diag);
  g_savedir = karta;
  FakeStr jsys{sys}, jhra{exe};
  const int N = 420;

  // ---- A: dynarec, kresleni ve vlakne ----
  j_denik();
  g_ps1_kresleni_volba = "auto";
  int boot = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  const bool interpret = psxCpu == &psxInt;
  char nast[400] = "";
  nap_ps1_nastaveni_popis(nast, sizeof(nast));
#if defined(__arm__) || defined(__aarch64__)
  const bool chciDynarec = true;     // telefon / TV: dynarec ari64
#else
  const bool chciDynarec = false;    // PC: jadro prelozene s DRC_DISABLE (dynarec appky je jen pro ARM)
#endif
  over(boot >= 0 && interpret == !chciDynarec && nap_ps1_kresleni_ve_vlakne() == 1,
       std::string("A1) jadro bezi: procesor PS1 ") + (interpret ? "INTERPRET" : "DYNAREC (prekladac do strojoveho kodu)") +
       ", kresleni ve vlakne " + (nap_ps1_kresleni_ve_vlakne() == 1 ? "ANO" : "NE") + " | " + nast);
  Beh a;
  kroky(N, a);
  const uint32_t hotovoA = ram32(0x80180000u), znackaA = ram32(0x8018000cu);
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);

  g_ps1_kresleni_volba = "disabled";
  int boot2 = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  Beh b;
  kroky(N, b);
  const uint32_t hotovoB = ram32(0x80180000u);
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);
  g_ps1_kresleni_volba = "auto";
  over(boot2 >= 0 && znackaA == 0xb332b332u && hotovoA > 300 && hotovoB > 300 && a.otisky.size() > 300 && b.otisky.size() > 300
       && a.sirka == 640 && a.vyska == 480 && !a.spatnaVelikost && !b.spatnaVelikost,
       "A2) program kresli a bezi ve vlakne i bez nej (snimku " + std::to_string(hotovoA) + " / " + std::to_string(hotovoB) + ", ruznych obrazu " +
       std::to_string(a.otisky.size()) + " / " + std::to_string(b.otisky.size()) + ", " + std::to_string(a.sirka) + "x" + std::to_string(a.vyska) + ")");

  // ---- O: ve vlakne = bez vlakna ----
  int spol = 0, jinych = 0;
  for (const auto &p : a.otisky) { auto q = b.otisky.find(p.first); if (q == b.otisky.end()) continue; spol++; if (q->second != p.second) jinych++; }
  int ctSpol = 0, ctJinych = 0;
  for (const auto &p : a.cteni) { auto q = b.cteni.find(p.first); if (q == b.cteni.end()) continue; ctSpol++; if (q->second != p.second) ctJinych++; }
  over(spol > 300 && jinych == 0 && ctSpol >= 30 && ctJinych == 0,
       "O1) obraz s kreslenim ve vlakne bajt po bajtu stejny jako bez nej: " + std::to_string(spol) + " snimku (" + std::to_string(jinych) +
       " ruznych), cteni VRAM " + std::to_string(ctSpol) + " (" + std::to_string(ctJinych) + " ruznych)");

  // ---- R: reference PC / srovnani ----
  if (rezim == "zapis") {
    std::ofstream o(ref);
    for (const auto &p : a.otisky) o << "S " << p.first << " " << hx(p.second) << "\n";
    for (const auto &p : a.cteni) o << "C " << p.first << " " << p.second << "\n";
    over((bool)o, "R0) reference zapsana: " + std::to_string(a.otisky.size()) + " snimku, " + std::to_string(a.cteni.size()) + " cteni VRAM -> " + ref);
  } else {
    std::map<int, std::string> rs;
    std::map<uint32_t, uint32_t> rc;
    std::ifstream in(ref);
    std::string t;
    while (in >> t) {
      if (t == "S") { int c; std::string h; in >> c >> h; rs[c] = h; }
      else if (t == "C") { uint32_t c, v; in >> c >> v; rc[c] = v; }
    }
    int rSpol = 0, rJinych = 0, prvni = -1;
    for (const auto &p : a.otisky) {
      auto q = rs.find(p.first);
      if (q == rs.end()) continue;
      rSpol++;
      if (q->second != hx(p.second)) { rJinych++; if (prvni < 0) prvni = p.first; }
    }
    int rcSpol = 0, rcJinych = 0;
    for (const auto &p : a.cteni) { auto q = rc.find(p.first); if (q == rc.end()) continue; rcSpol++; if (q->second != p.second) rcJinych++; }
    over(!rs.empty() && rSpol > 300 && rJinych == 0,
         "R1) OBRAZ STEJNY JAKO NA PC: " + std::to_string(rSpol) + " snimku (podle cisla v obraze) bajt po bajtu stejnych jako reference z PC" +
         (rJinych ? ", RUZNYCH " + std::to_string(rJinych) + " (prvni c. " + std::to_string(prvni) + ")" : std::string("")) +
         " - dynarec ARM = interpret, gpu_neon NEON assembler = SIMD C");
    over(rcSpol >= 30 && rcJinych == 0, "R2) cteni VRAM zpet (GPUREAD) stejne jako na PC: " + std::to_string(rcSpol) + " cteni, " + std::to_string(rcJinych) + " ruznych");
  }

  // ---- S: ulozeni a nahrani pozice ----
  boot = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  Beh s0; kroky(120, s0);
  std::vector<uint8_t> poz(retro_serialize_size());
  bool ulozeno, nahrano;
  { std::lock_guard<std::mutex> l(g_core_mutex); ulozeno = retro_serialize(poz.data(), poz.size()); }
  Beh s1; kroky(90, s1);
  { std::lock_guard<std::mutex> l(g_core_mutex); nahrano = retro_unserialize(poz.data(), poz.size()); }
  Beh s2; kroky(90, s2);
  int sSpol = 0, sJinych = 0;
  for (const auto &p : s1.otisky) { auto q = s2.otisky.find(p.first); if (q == s2.otisky.end()) continue; sSpol++; if (q->second != p.second) sJinych++; }
  over(boot >= 0 && ulozeno && nahrano && sSpol > 60 && sJinych == 0,
       "S1) pozice ulozena a nahrana (" + std::to_string(poz.size() / 1024) + " kB): po nahrani " + std::to_string(sSpol) + " snimku stejnych jako poprve (" +
       std::to_string(sJinych) + " ruznych)");
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);
  std::printf("   cas na snimek: ve vlakne %.2f ms, bez vlakna %.2f ms (%s)\n", a.krokMs, b.krokMs, architektura());
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
