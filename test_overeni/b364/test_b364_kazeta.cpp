// B364: CSAVE po resetu POKEY (SKCTL 0 -> 3) a CLOAD z toho WAV - jadro Atari bez Androidu.
//
// Rene 25. 11.: vlastni program (3D + dva POKEY) -> RUN -> Q -> CSAVE -> NEW -> CLOAD = ERROR 143, OS z pasky
// precetl jen ~20 bajtu (chyb ramce 106) na 21.6 s. Pricina: program pri hledani druheho POKEY zapise SKCTL = 0
// a pak 3 (init rezim POKEY - bezne u detekce stereo). Jadro do B363 pritom dalo linku SIO DATA OUT na MEZERU (0)
// a ta tam zustala az do prvniho vyslaneho bajtu - CSAVE pak nahral 20 s uvodniho tonu jako mezeru (3,97 kHz misto
// 5,33 kHz) a prvni zaznam (hlavicka programu) nesel precist. Skutecny POKEY si v init rezimu drzi posledni
// vyslany bit (Altirra Hardware Reference Manual, POKEY - serial port reset) = v klidu MARK.
//
// Test: (1) bez resetu POKEY, (2) program s POKE 53775,0:POKE 53775,3, (3) totez strojovym kodem z USR (SEI, SKCTL
// 0, cteni RANDOM, SKCTL 3, CLI - jako stereo detekce) - kazdy: RUN, CSAVE -> WAV (uvodni ton, zaznamy, druh) ->
// NEW -> CLOAD ve stejnem stroji (jako Rene) -> LIST = puvodni program. Volitelne: Reneho program (cesta v argv[1],
// jen na PC - neni v repozitari): napsat (jako VLOZIT z PC), RUN, menu 1 2 3 4 S S, Q, CSAVE, CLOAD.
//
// Preklad (z test_overeni/b364; pomocne funkce z ../b292/test_core.cpp):
//   g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -o test_b364_kazeta test_b364_kazeta.cpp
//   ./test_b364_kazeta [reneho_program.txt|- [stara_nahravka.wav]]   -> VYSTUP_test_b364_kazeta.txt
#define main b292_core_main
#include "../b292/test_core.cpp"
#undef main

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  (ok ? g_ok : g_chyb)++;
}
static void beh(Machine &m, PutHook &h, int n) { for (int i = 0; i < n; i++) snimekH(m, h); }

// frekvence tonu ve WAV (pruchody nulou) v useku [t0, t1) s
static double ton(const std::vector<int16_t> &pcm, double t0, double t1) {
  const size_t a = (size_t)(t0 * 44100), b = std::min(pcm.size(), (size_t)(t1 * 44100));
  int zc = 0;
  for (size_t i = a + 1; i < b; i++) if ((pcm[i - 1] < 0) != (pcm[i] < 0)) zc++;
  return b > a ? zc / 2.0 / ((b - a) / 44100.0) : 0;
}

// RUN (kdyz je program), CSAVE do WAV, NEW, CLOAD ve stejnem stroji, LIST porovnat
static std::vector<int16_t> g_posledniWav;   // pcm posledniho CSAVE (oprava nahravky z B363)
static double g_posledniPrvni = 0;
static void kazeta(const std::string &nazev, Machine *m, PutHook &h, const std::string &listPred) {
  m->tapeCapture = true;
  CsaveRecorder rec;
  {
    float t[882]; m->genTape(t, 882);
    TypeQueue tq; tq.addText("CSAVE");
    bool enterPoslan = false; int poBeep = 0;
    for (int f = 0; f < 50 * 900 && !rec.maHotovo; f++) {
      tq.step(*m); snimekH(*m, h);
      m->genTape(t, 882);
      for (int i = 0; i < 882; i++) t[i] *= 0.8f;
      rec.snimek(*m, t, 882);
      if (tq.empty() && !enterPoslan && ++poBeep == 150) { m->klavesa(12); enterPoslan = true; }
    }
  }
  if (!rec.maHotovo) { over(false, nazev + ": CSAVE - WAV nevznikl"); return; }
  std::vector<uint8_t> wav;
  {
    const uint32_t nb = (uint32_t)rec.hotovo.size() * 2;
    uint8_t hd[44] = {'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,1,0,0x44,0xAC,0,0,0x88,0x58,1,0,2,0,16,0,'d','a','t','a',0,0,0,0};
    uint32_t r = 36 + nb; std::memcpy(hd + 4, &r, 4); std::memcpy(hd + 40, &nb, 4);
    wav.assign(hd, hd + 44);
    for (int16_t v : rec.hotovo) { wav.push_back((uint8_t)(v & 0xFF)); wav.push_back((uint8_t)((v >> 8) & 0xFF)); }
  }
  TapeImage img; std::string err;
  if (!napTapeFromWav(wav.data(), wav.size(), img, err)) { over(false, nazev + ": WAV - " + err); return; }
  const double uvod = ton(rec.hotovo, 5.0, 6.0);
  g_posledniWav = rec.hotovo; g_posledniPrvni = img.prvniCas;
  const long long ocekZaznamu = (m->seroutPocet - rec.serout0) / 132;   // zaznam = 2x $55, ridici bajt, 128 dat, soucet
  char b[400];
  std::snprintf(b, sizeof(b), "%s: CSAVE -> WAV %.1f s, uvodni ton %.0f Hz (MARK 5327 Hz, mezera 3995 Hz), zaznamu %lld z %lld (OK %lld, spatne %lld), druh: %s",
                nazev.c_str(), img.seconds(), uvod, img.records600, ocekZaznamu, img.recordsOk600, img.recordsBad600, napTapeDruh(img));
  over(uvod > 5000 && uvod < 5600 && img.druh == TapeImage::BASIC && img.recordsBad600 == 0 && img.recordsOk600 == ocekZaznamu, b);
  // NEW a CLOAD ve stejnem stroji (jako Rene)
  pisH(*m, h, "NEW", 20);
  m->tape.img = img; m->tape.loaded = true; m->tape.pos = 0; m->tape.play = true; m->tape.name = nazev;
  const long long rx0 = m->tape.bytesRx, fr0 = m->tape.framingRx;
  {
    TypeQueue tq; tq.addText("CLOAD");
    bool enter = false; int cekej = 0;
    for (int f = 0; f < 50 * 900; f++) {
      tq.step(*m); snimekH(*m, h);
      if (tq.empty() && !enter && ++cekej == 100) { m->klavesa(12); enter = true; cekej = 0; }
      if (enter && !m->motorOn() && ++cekej > 100) {
        const std::string sc = obrazovka(*m);
        if (sc.find("READY", sc.rfind("CLOAD")) != std::string::npos || sc.find("ERROR") != std::string::npos) break;
      }
    }
  }
  const std::string sc = obrazovka(*m);
  const bool chyba = sc.find("ERROR", sc.rfind("CLOAD")) != std::string::npos;
  const std::string listPo = listH(*m, h);
  std::snprintf(b, sizeof(b), "%s: NEW + CLOAD ve stejnem stroji - OS precetl %lld B, chyb ramce %lld, %s, LIST po CLOAD %s",
                nazev.c_str(), m->tape.bytesRx - rx0, m->tape.framingRx - fr0, chyba ? "ERROR" : "READY",
                normalizuj(listPo) == normalizuj(listPred) ? "STEJNY jako pred CSAVE" : "JINY");
  over(!chyba && m->tape.framingRx == fr0 && normalizuj(listPo) == normalizuj(listPred), b);
}

static Machine *basicSProgramem(PutHook &h, const std::string &program) {
  Machine *m = startBasic(false, h);
  pisH(*m, h, program, 50);
  return m;
}

int main(int argc, char **argv) {
  std::printf("B364 - CSAVE po resetu POKEY (SKCTL 0 -> 3) a CLOAD z toho WAV (Atari 130XE, jadro bez Androidu)\n");
  {
    PutHook h;
    Machine *m = basicSProgramem(h, "10 REM BEZ RESETU POKEY\n20 FOR I=1 TO 40:? \"RADEK \";I:NEXT I\n30 END\n");
    const std::string l = listH(*m, h);
    pisH(*m, h, "RUN", 150);
    kazeta("bez resetu POKEY", m, h, l);
    delete m;
  }
  {
    PutHook h;
    Machine *m = basicSProgramem(h, "10 REM RESET POKEY JAKO STEREO DETEKCE\n20 POKE 53775,0:POKE 53775,3\n30 FOR I=1 TO 40:? \"RADEK \";I:NEXT I\n40 END\n");
    const std::string l = listH(*m, h);
    pisH(*m, h, "RUN", 150);
    over(m->skctl == 3, "program zapsal SKCTL 0 a 3 (POKEY prosel init rezimem), SKCTL ted $" + std::to_string(m->skctl));
    kazeta("POKE 53775,0:POKE 53775,3", m, h, l);
    delete m;
  }
  {
    // strojovy kod jako v Reneho programu: PLA, SEI, SKCTL2=3, SKCTL=0, 8x LDA $D21A / CMP $D21A, SKCTL=3, CLI, RTS
    PutHook h;
    Machine *m = basicSProgramem(h,
        "10 REM USR - DETEKCE DRUHEHO POKEY\n"
        "20 FOR I=1536 TO 1576:READ A:POKE I,A:NEXT I\n"
        "30 X=USR(1536):? \"PROBE \";X\n"
        "40 DATA 104,120,169,3,141,31,210,169,0,141,15,210,162,0,160,8,173,26,210,205,26,210,208,5,136,208,245\n"
        "50 DATA 240,2,162,1,169,3,141,15,210\n"
        "60 DATA 141,31,210,88,96\n");
    const std::string l = listH(*m, h);
    pisH(*m, h, "RUN", 200);
    kazeta("USR: SEI, SKCTL 0, RANDOM, SKCTL 3, CLI", m, h, l);
    delete m;
  }
  {
    // nahravka jako z B363: uvodni ton az do zacatku prvniho zaznamu jako MEZERA (3995 Hz) - appka ji pri vlozeni
    // opravi (dlouha mezera = klid) a CLOAD ji v novem stroji nahraje
    PutHook h;
    const std::string prog = "10 REM NAHRAVKA Z B363\n20 FOR I=1 TO 40:? \"RADEK \";I:NEXT I\n30 END\n";
    Machine *m = basicSProgramem(h, prog);
    const std::string l = listH(*m, h);
    kazeta("vzor pro nahravku z B363", m, h, l);
    delete m;
    std::vector<int16_t> pcm = g_posledniWav;
    const size_t konec = (size_t)(g_posledniPrvni * 44100);
    for (size_t i = 0; i < konec && i < pcm.size(); i++) {
      const double faze = std::fmod(i * 3995.0 / 44100.0, 1.0);
      pcm[i] = (int16_t)(faze < 0.5 ? 26214 : -26214);
    }
    std::vector<uint8_t> wav;
    const uint32_t nb = (uint32_t)pcm.size() * 2;
    uint8_t hd[44] = {'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,1,0,0x44,0xAC,0,0,0x88,0x58,1,0,2,0,16,0,'d','a','t','a',0,0,0,0};
    uint32_t r = 36 + nb; std::memcpy(hd + 4, &r, 4); std::memcpy(hd + 40, &nb, 4);
    wav.assign(hd, hd + 44);
    for (int16_t v : pcm) { wav.push_back((uint8_t)(v & 0xFF)); wav.push_back((uint8_t)((v >> 8) & 0xFF)); }
    TapeImage img; std::string err;
    const bool nacteno = napTapeFromWav(wav.data(), wav.size(), img, err);
    char b[300];
    std::snprintf(b, sizeof(b), "nahravka z B363 (uvodni ton %.0f Hz = mezera do %.1f s): pri vlozeni opraveno %d useku (%.1f s), zaznamu %lld (OK %lld), druh: %s",
                  ton(pcm, 5.0, 6.0), g_posledniPrvni, img.opravenoMezer, img.opravenoSekund, img.records600, img.recordsOk600, napTapeDruh(img));
    over(nacteno && img.opravenoMezer == 1 && img.druh == TapeImage::BASIC && img.recordsBad600 == 0, b);
    PutHook h2;
    Machine *b2 = startBasic(false, h2);
    b2->tape.img = img; b2->tape.loaded = true; b2->tape.pos = 0; b2->tape.play = true; b2->tape.name = "b363";
    TypeQueue tq; tq.addText("CLOAD");
    bool enter = false; int cekej = 0;
    for (int f = 0; f < 50 * 300; f++) {
      tq.step(*b2); snimekH(*b2, h2);
      if (tq.empty() && !enter && ++cekej == 100) { b2->klavesa(12); enter = true; cekej = 0; }
      if (enter && !b2->motorOn() && ++cekej > 100) {
        const std::string sc = obrazovka(*b2);
        if (sc.find("READY", sc.rfind("CLOAD")) != std::string::npos || sc.find("ERROR") != std::string::npos) break;
      }
    }
    const std::string lb = listH(*b2, h2);
    std::snprintf(b, sizeof(b), "nahravka z B363: CLOAD v novem stroji - OS precetl %lld B, chyb ramce %lld, LIST %s",
                  b2->tape.bytesRx, b2->tape.framingRx, normalizuj(lb) == normalizuj(l) ? "STEJNY jako puvodni program" : "JINY");
    over(b2->tape.framingRx == 0 && normalizuj(lb) == normalizuj(l), b);
    delete b2;
    // dobra kazeta se nemeni (zadny dlouhy usek mezery)
    TapeImage img2; std::vector<uint8_t> wav2(hd, hd + 44);
    for (int16_t v : g_posledniWav) { wav2.push_back((uint8_t)(v & 0xFF)); wav2.push_back((uint8_t)((v >> 8) & 0xFF)); }
    napTapeFromWav(wav2.data(), wav2.size(), img2, err);
    over(img2.opravenoMezer == 0, "dobra nahravka (uvodni ton MARK) se pri vlozeni neopravuje: " + std::to_string(img2.opravenoMezer) + " useku");
  }
  if (argc > 2) {
    // skutecna nahravka ze stareho jadra (jen lokalne): vlozit, CLOAD v novem stroji
    std::vector<uint8_t> wav = nacti(argv[2]);
    TapeImage img; std::string err;
    const bool nacteno = napTapeFromWav(wav.data(), wav.size(), img, err);
    PutHook h2;
    Machine *b2 = startBasic(false, h2);
    b2->tape.img = img; b2->tape.loaded = true; b2->tape.pos = 0; b2->tape.play = true; b2->tape.name = "stara";
    TypeQueue tq; tq.addText("CLOAD");
    bool enter = false; int cekej = 0;
    for (int f = 0; f < 50 * 600; f++) {
      tq.step(*b2); snimekH(*b2, h2);
      if (tq.empty() && !enter && ++cekej == 100) { b2->klavesa(12); enter = true; cekej = 0; }
      if (enter && !b2->motorOn() && ++cekej > 100) {
        const std::string sc = obrazovka(*b2);
        if (sc.find("READY", sc.rfind("CLOAD")) != std::string::npos || sc.find("ERROR") != std::string::npos) break;
      }
    }
    const std::string sc = obrazovka(*b2);
    char b[300];
    std::snprintf(b, sizeof(b), "stara nahravka (%s): opraveno %d useku (%.1f s), zaznamu %lld (OK %lld), druh %s; CLOAD - OS precetl %lld B, chyb ramce %lld, %s",
                  argv[2], img.opravenoMezer, img.opravenoSekund, img.records600, img.recordsOk600, napTapeDruh(img), b2->tape.bytesRx,
                  b2->tape.framingRx, sc.find("ERROR", sc.rfind("CLOAD")) != std::string::npos ? "ERROR" : "READY");
    over(nacteno && b2->tape.framingRx == 0 && sc.find("ERROR", sc.rfind("CLOAD")) == std::string::npos, b);
    delete b2;
  }
  if (argc > 1 && std::string(argv[1]) != "-") {
    // Reneho program (jen lokalne): napsat jako VLOZIT z PC (bez NEW), RUN, menu, Q, CSAVE, CLOAD
    std::vector<uint8_t> v = nacti(argv[1]);
    std::string text(v.begin(), v.end());
    PutHook h;
    Machine *m = startBasic(false, h);
    PsaniProgramu ps;
    ps.start((const uint8_t *)text.data(), text.size(), false, "vlozeno z PC", false, false);
    for (int f = 0; f < 50 * 900 && (ps.aktivni() || f < 2); f++) {
      if (ps.pise()) m->runFrameRadky([&](Machine &mm) { ps.radek(mm); });
      else snimekH(*m, h);
      ps.poSnimku(*m);
    }
    const std::string l = listH(*m, h);
    pisH(*m, h, "RUN", 10);
    int f = 0;
    for (; f < 50 * 400; f++) { snimekH(*m, h); if (f % 50 == 0 && obrazovka(*m).find("SELECT:") != std::string::npos) break; }
    const bool menu = obrazovka(*m).find("SELECT:") != std::string::npos, dual = obrazovka(*m).find("DUAL POKEY FOUND") != std::string::npos;
    const int k[] = {31, 30, 26, 24, 62, 62, 47};       // 1 2 3 4 S S Q
    for (int kk : k) { m->klavesa(kk); beh(*m, h, 3); beh(*m, h, 50 * 3); }
    over(menu && dual, std::string("Reneho program: napsan (") + std::to_string(ps.radku) + " radku), RUN -> menu po " + std::to_string(f / 50) +
                           " s, " + (dual ? "DUAL POKEY FOUND" : "druhy POKEY NENALEZEN") + ", klavesy 1 2 3 4 S S Q");
    kazeta("Reneho program", m, h, l);
    delete m;
  }
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
