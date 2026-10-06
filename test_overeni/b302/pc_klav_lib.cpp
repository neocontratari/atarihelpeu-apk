// B302: knihovna pro test "prohlizec -> C++ Atari" (test_b302_prohlizec.py).
// Skutecne jadro (nap_atari_machine.h) + skutecna klavesnice pocitace
// (nap_atari_pc_klavesnice.h) - stejne jako v appce, jen bez Androidu:
// Python server dela to, co v appce Java (/status, /klavesa), Chromium
// (Playwright) to, co prohlizec na PC u TV.
//   g++ -std=c++17 -O2 -shared -fPIC -I../../app/src/main/cpp/atari -o libpcklav.so pc_klav_lib.cpp
#include <chrono>
#include <mutex>
#include <string>
#include <vector>
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
#include "nap_atari_runtime.h"
#include "nap_atari_pc_klavesnice.h"
using namespace nap;

static std::mutex g_m;
static Machine *g_stroj = nullptr;
static PcKlavesnice g_kl;
static PcPrehravac g_hr;
static int g_joy = 0, g_kon = 0;
static std::string g_ret, g_obr, g_log;
static int g_ticho = 0;
static long long ted_ms() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
static void akce(const std::vector<PcAkce> &a) {
  for (const auto &x : a) {
    if (x.t == PcAkce::JOY) g_joy = x.v & 31;
    else if (x.t == PcAkce::KONZOLE) g_kon = x.v & 7;
    else g_hr.pridej(x);
  }
}

static std::string obrazovka(Machine &m) {
  int sav = m.mem.ram[0x58] | (m.mem.ram[0x59] << 8);
  std::string out;
  for (int r = 0; r < 24; r++) {
    std::string line;
    for (int c = 0; c < 40; c++) {
      int v = m.peek((uint16_t)(sav + r * 40 + c)) & 0x7F;
      int a = v < 64 ? v + 32 : (v < 96 ? v - 64 : v);
      if (a < 32 || a > 126) a = ' ';
      line.push_back((char)a);
    }
    while (!line.empty() && line.back() == ' ') line.pop_back();
    out += line + "\n";
  }
  return out;
}

extern "C" {
// studeny start s BASICem az do READY
int pk_init() {
  std::lock_guard<std::mutex> l(g_m);
  delete g_stroj;
  g_stroj = new Machine();
  g_stroj->view = new AnticView();
  g_stroj->mem.os = NAP_OS_ROM; g_stroj->mem.bas = NAP_BASIC_ROM;
  g_stroj->coldInit();
  g_stroj->consol = 7;
  g_stroj->cpu.reset();
  for (int f = 0; f < 400; f++) { g_stroj->runFrame(); if (f > 30 && obrazovka(*g_stroj).find("READY") != std::string::npos) return 1; }
  return 0;
}
// /klavesa (to, co v appce dela Java napTvWebKlavesa -> devPcKlavesaNative)
const char *pk_klavesa(const char *code, const char *znak, int mod, int dolu) {
  std::lock_guard<std::mutex> l(g_m);
  if (std::string(code) == "STAV") { g_ret = g_kl.hrani ? "HRANI" : "PSANI"; return g_ret.c_str(); }
  std::vector<PcAkce> a;
  g_kl.zprava(ted_ms());                       // hlidani spojeni (jako v appce)
  g_ret = pcAscii(g_kl.udalost(code, znak, mod, dolu != 0, a), 64);
  akce(a);
  return g_ret.c_str();
}
// jeden snimek (emulacni vlakno v appce): klavesy v tempu, joystick, konzole
void pk_snimek() {
  std::lock_guard<std::mutex> l(g_m);
  if (!g_stroj) return;
  std::vector<PcAkce> a;
  if (g_kl.hlidej(ted_ms(), a)) { g_ticho++; akce(a); }   // prohlizec se odmlcel - vse pustit (jako v appce)
  std::vector<int> st;
  g_hr.krok(*g_stroj, &st);
  for (int v : st) g_log += (v < 0 ? std::string("<BREAK>") : napScanText(v));
  g_stroj->porta = (0xF0 | (~g_joy & 15)) & 0xFF;
  g_stroj->trig[0] = (g_joy & 16) ? 0 : 1;
  g_stroj->consol = 7 & ~g_kon;
  g_stroj->runFrame();
}
const char *pk_obrazovka() { std::lock_guard<std::mutex> l(g_m); g_obr = g_stroj ? obrazovka(*g_stroj) : ""; return g_obr.c_str(); }
const char *pk_log() { std::lock_guard<std::mutex> l(g_m); g_ret = g_log; g_log.clear(); return g_ret.c_str(); }
int pk_joy() { std::lock_guard<std::mutex> l(g_m); return g_joy; }
int pk_kon() { std::lock_guard<std::mutex> l(g_m); return g_kon; }
int pk_klav_drzena() { std::lock_guard<std::mutex> l(g_m); return g_stroj && !(g_stroj->skstat & 4) ? g_stroj->kbcode : -1; }
int pk_fronta() { std::lock_guard<std::mutex> l(g_m); return (int)g_hr.q.size(); }
int pk_ticho() { std::lock_guard<std::mutex> l(g_m); return g_ticho; }
}
