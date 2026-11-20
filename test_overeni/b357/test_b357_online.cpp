// test_overeni/b357/test_b357_online.cpp
// B357: ONLINE HRANI KONEC-KONCU - skutecny klient appky (nap_online_klient.h: WebSocket v C++ + relace) proti
// skutecne tride NapOnline enginu v12.4 (mono, test/NapOnlineTest.cs: WebSocket.CreateFromStream = implementace
// .NET, nezavisla na klientovi). Dva "telefony" zalozi / najdou / pripoji hru a odehraji 360 snimku: obraz musi byt
// v kazdem snimku stejny. Plus seznam her, chybove odpovedi enginu (404 / 409 / 400), velka zprava, odchod hrace.
//   g++ -std=c++17 -O2 -pthread -Wall -Wextra -I../../app/src/main/cpp -o test_b357_online test_b357_online.cpp
//   mono NapOnlineTest.exe --port 18781 --ticho 5 &   (engine; konci radkem "konec")
//   ./test_b357_online http://127.0.0.1:18781          -> VYSTUP_test_b357_online.txt
#include "online/nap_online_klient.h"

#include <cstdio>
#include <thread>

using namespace nap::online;

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}

static std::string g_adresa;

// obycejny GET (seznam her) - jako appka pres HttpURLConnection
static std::string get(const std::string &cesta, int *kod = nullptr) {
  std::string host; int port = 0;
  WebSocket::rozloz(g_adresa, host, port);
  int s = socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in a{}; a.sin_family = AF_INET; a.sin_port = htons((uint16_t)port); inet_pton(AF_INET, host.c_str(), &a.sin_addr);
  if (connect(s, (sockaddr *)&a, sizeof(a)) != 0) { close(s); return ""; }
  std::string q = "GET " + cesta + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
  send(s, q.data(), q.size(), 0);
  std::string r; char b[4096]; ssize_t n;
  while ((n = recv(s, b, sizeof(b), 0)) > 0) r.append(b, (size_t)n);
  close(s);
  if (kod) *kod = r.size() > 12 ? std::atoi(r.c_str() + 9) : 0;
  const size_t i = r.find("\r\n\r\n");
  return i == std::string::npos ? "" : r.substr(i + 4);
}

static const char *OTISK = "0123456789abcdef0123456789abcdef01234567";

static Ahoj ahoj(const char *jmeno) {
  Ahoj a;
  a.jadro = J_SEGA; a.nastaveni = 7; a.jmeno = jmeno;
  for (int i = 0; i < 20; i++) a.hra[i] = (uint8_t)(0x10 + i);
  return a;
}

static std::vector<std::string> podpis(const char *zarizeni) {
  return {"User-Agent: NapVision/B357 (Android test; PC)", std::string("X-NaP-Zarizeni: ") + zarizeni, "X-NaP-Cas: 1763600000", "X-NaP-Podpis: 00"};
}

struct Stroj {
  uint64_t s = 0x1234567;
  uint8_t ram[4096] = {};
  uint64_t krok(uint32_t p1, uint32_t p2) {
    for (int i = 0; i < 64; i++) { s = s * 6364136223846793005ULL + 1442695040888963407ULL + p1 * 31 + p2 * 17; ram[(s >> 20) % 4096] ^= (uint8_t)(s >> 40); }
    return otisk(ram, sizeof(ram), s);
  }
};
static uint32_t skript(int hrac, uint32_t f) { return hrac == 1 ? ((f / 7) * 2654435761u) & 0xFFF : (((f / 11) * 40503u) ^ 0x5A5) & 0xFFF; }

// vlakno emulace jednoho telefonu - jako bude v jadre Segy
static void emulace(OnlineKlient &k, int hrac, uint32_t celkem, std::vector<uint64_t> &obrazy) {
  const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(20);
  while (!k.vezmiStart()) {
    if (k.faze() == OnlineKlient::Faze::KONEC || std::chrono::steady_clock::now() > limit) return;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  std::shared_ptr<Relace> r = k.relace();
  Stroj st;
  auto dalsi = std::chrono::steady_clock::now();
  for (uint32_t f = 0; f < celkem; f++) {
    r->lokalni(f, skript(hrac, f));
    uint32_t p1 = 0, p2 = 0;
    int v;
    while ((v = r->vstupy(f, p1, p2, 50)) == 0) {}
    if (v < 0) return;
    obrazy.push_back(st.krok(p1, p2));
    r->otiskStavu(f, otisk(st.ram, sizeof(st.ram), st.s));
    dalsi += std::chrono::microseconds(16667);
    if (dalsi > std::chrono::steady_clock::now()) std::this_thread::sleep_until(dalsi);
    else dalsi = std::chrono::steady_clock::now();
  }
}

static bool cekej(std::function<bool()> f, int ms) {
  const auto konec = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
  while (!f()) { if (std::chrono::steady_clock::now() > konec) return false; std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
  return true;
}

int main(int argc, char **argv) {
  g_adresa = argc > 1 ? argv[1] : "http://127.0.0.1:18781";

  std::printf("== 1) seznam her na enginu: zatim zadna\n");
  int kod = 0;
  std::string s = get("/api/v1/online/mistnosti", &kod);
  over(kod == 200 && s.find("\"mistnosti\":[]") != std::string::npos, "GET /api/v1/online/mistnosti -> " + std::to_string(kod) + " " + s.substr(0, 60));

  std::printf("== 2) chyby enginu se dostanou do appky (HTTP + JSON s duvodem)\n");
  {
    WebSocket w; std::string chyba; int k = 0;
    bool spojeno = w.spoj(g_adresa, "/api/v1/online/pripoj?kod=0000&jmeno=bracha", podpis("tel-b"), 3000, chyba, &k);
    over(!spojeno && k == 404 && chyba.find("mistnost_neni") != std::string::npos, "pripojeni k neexistujici hre -> " + chyba.substr(0, 110));
    spojeno = w.spoj(g_adresa, "/api/v1/online/zaloz?system=sega&otisk=spatny", podpis("tel-a"), 3000, chyba, &k);
    over(!spojeno && k == 400 && chyba.find("otisk") != std::string::npos, "zalozeni bez otisku hry -> HTTP " + std::to_string(k));
    spojeno = w.spoj(g_adresa, "/api/v1/online/zaloz?system=nes&otisk=" + std::string(OTISK), podpis("tel-a"), 3000, chyba, &k);
    over(!spojeno && k == 400, "neznamy system -> HTTP " + std::to_string(k));
  }

  std::printf("== 3) WebSocket primo: zalozit, cislo hry, seznam, druhy se pripoji, velka zprava pres engine\n");
  std::string mistnost;
  {
    WebSocket a, b;
    std::string chyba;
    over(a.spoj(g_adresa, std::string("/api/v1/online/zaloz?system=sega&hra=Test%20hra%20%C5%A1%C4%8D&hra_id=sega-0123456789abcdef&otisk=") + OTISK + "&nastaveni=7&jmeno=Ren%C3%A9",
                podpis("tel-a"), 3000, chyba), "hrac 1 zalozil hru (WebSocket 101, Sec-WebSocket-Accept od .NET sedi) " + chyba);
    std::vector<uint8_t> z; uint8_t op = 0;
    over(a.prijmi(z, 3000, &op) == 1 && op == 1, "engine poslal textovou zpravu");
    const std::string j(z.begin(), z.end());
    mistnost = jsonText(j, "kod");
    over(jsonText(j, "typ") == "mistnost" && mistnost.size() == 4, "cislo hry: " + mistnost + " (" + j + ")");
    s = get("/api/v1/online/mistnosti");
    over(s.find("\"kod\":\"" + mistnost + "\"") != std::string::npos && jsonText(s, "hra") == "Test hra \xC5\xA1\xC4\x8D" && jsonText(s, "hrac") == "Ren\xC3\xA9"
         && jsonText(s, "hra_id") == "sega-0123456789abcdef" && jsonText(s, "otisk") == OTISK, "seznam: hra ceka, nazev s diakritikou, hrac, id na serveru, otisk");
    over(b.spoj(g_adresa, "/api/v1/online/pripoj?kod=" + mistnost + "&jmeno=bracha", podpis("tel-b"), 3000, chyba), "hrac 2 se pripojil " + chyba);
    over(a.prijmi(z, 3000, &op) == 1 && op == 1 && jsonText(std::string(z.begin(), z.end()), "jmeno") == "bracha", "hrac 1 se dozvi: pripojil se bracha");
    over(b.prijmi(z, 3000, &op) == 1 && op == 1 && jsonText(std::string(z.begin(), z.end()), "jmeno") == "Ren\xC3\xA9"
         && jsonText(std::string(z.begin(), z.end()), "otisk") == OTISK, "hrac 2 se dozvi: hra od Rene + otisk hry (at vi, co pustit)");
    {
      WebSocket c; int k = 0;
      const bool spojeno = c.spoj(g_adresa, "/api/v1/online/pripoj?kod=" + mistnost + "&jmeno=treti", podpis("tel-c"), 3000, chyba, &k);
      over(!spojeno && k == 409 && chyba.find("obsazeno") != std::string::npos, "treti uz se nevejde -> HTTP " + std::to_string(k) + " obsazeno");
    }
    s = get("/api/v1/online/mistnosti");
    over(s.find(mistnost) == std::string::npos, "hrajici hra uz v seznamu neni");
    // velka zprava 300 KB (64bitova delka, engine ji prijima po 64 KB a posle celou)
    std::vector<uint8_t> velka(300000);
    for (size_t i = 0; i < velka.size(); i++) velka[i] = (uint8_t)(i * 7 + 3);
    over(a.posli(velka), "hrac 1 poslal 300 KB");
    over(b.prijmi(z, 5000, &op) == 1 && op == 2 && z == velka, "hrac 2 dostal presne tech 300 KB (binarne, beze zmeny)");
    const uint8_t mala[3] = {VSTUP, 1, 2};
    b.posli(mala, 3);
    over(a.prijmi(z, 3000, &op) == 1 && op == 2 && z.size() == 3 && z[0] == VSTUP, "a zpatky mala zprava 3 B");
    b.zavri();
    over(a.prijmi(z, 3000, &op) == 1 && op == 1 && jsonText(std::string(z.begin(), z.end()), "typ") == "odesel", "hrac 2 zavrel -> hrac 1 dostane {\"typ\":\"odesel\"}");
    over(a.prijmi(z, 3000, &op) == -1, "a engine spojeni zavrel");
  }

  std::printf("== 4) dve appky (OnlineKlient): zalozit, najit v seznamu, pripojit, odehrat 360 snimku\n");
  {
    OnlineKlient ka, kb;
    OnlineKlient::Nastaveni na;
    na.adresa = g_adresa;
    na.cesta = std::string("/api/v1/online/zaloz?system=sega&hra=Sonic&otisk=") + OTISK + "&nastaveni=7&jmeno=Rene";
    na.hlavicky = podpis("tel-a"); na.hrac1 = true; na.ja = ahoj("Rene");
    ka.start(na);
    const bool ceka = cekej([&] { return ka.faze() == OnlineKlient::Faze::CEKA_NA_HRACE && !ka.kodMistnosti().empty(); }, 3000);
    over(ceka, "appka hrace 1: " + ka.popis());
    s = get("/api/v1/online/mistnosti");
    const std::string k = jsonText(s, "kod");
    over(k == ka.kodMistnosti(), "appka hrace 2 vidi hru v seznamu: cislo " + k + ", hra " + jsonText(s, "hra"));
    OnlineKlient::Nastaveni nb = na;
    nb.cesta = "/api/v1/online/pripoj?kod=" + k + "&jmeno=bracha"; nb.hlavicky = podpis("tel-b"); nb.hrac1 = false; nb.ja = ahoj("bracha");
    kb.start(nb);
    std::vector<uint64_t> oa, ob;
    std::thread ea([&] { emulace(ka, 1, 360, oa); }), eb([&] { emulace(kb, 2, 360, ob); });
    ea.join(); eb.join();
    over(oa.size() == 360 && oa == ob, "oba 360 snimku, KAZDY obraz stejny (a " + std::to_string(oa.size()) + ", b " + std::to_string(ob.size()) + ")");
    std::shared_ptr<Relace> ra = ka.relace(), rb = kb.relace();
    over(ra && rb && ra->overenychOtisku() >= 5 && rb->overenychOtisku() >= 5, "otisky stavu overeny (a " + std::to_string(ra ? ra->overenychOtisku() : 0) + "x)");
    over(ra && ra->zpozdeni() == rb->zpozdeni() && ra->zpozdeni() <= 3, "pres engine na PC: zpozdeni D = " + std::to_string(ra ? ra->zpozdeni() : 0) + ", odezva " + std::to_string(ra ? ra->odezvaMs() : 0) + " ms");
    over(ka.jmenoDruheho() == "bracha" && kb.jmenoDruheho() == "Rene" && ka.popis().find("ONLINE S bracha") == 0, "stav pro hrace: " + ka.popis());
    kb.stop();
    const bool skoncil = cekej([&] { return ka.faze() == OnlineKlient::Faze::KONEC; }, 5000);
    over(skoncil && ka.chyba() == popisDuvodu(D_HRAC), "hrac 2 skoncil -> u hrace 1: " + ka.popis());
    s = get("/api/v1/online/mistnosti");
    over(s.find("\"mistnosti\":[]") != std::string::npos, "seznam her je zase prazdny");
  }

  std::printf("== 5) appka hrace 1 hru zrusi drive, nez nekdo prijde\n");
  {
    OnlineKlient ka;
    OnlineKlient::Nastaveni na;
    na.adresa = g_adresa;
    na.cesta = std::string("/api/v1/online/zaloz?system=sega&hra=Sonic&otisk=") + OTISK + "&jmeno=Rene";
    na.hlavicky = podpis("tel-a"); na.ja = ahoj("Rene");
    ka.start(na);
    cekej([&] { return !ka.kodMistnosti().empty(); }, 3000);
    const std::string k = ka.kodMistnosti();
    ka.stop();
    over(cekej([&] { return get("/api/v1/online/mistnosti").find(k) == std::string::npos; }, 3000), "zrusena hra " + k + " zmizela ze seznamu");
  }

  // B357 (po prvni verzi): hrac odejde, kdyz se appka teprve spojuje - stop() nesmi cekat na timeout spojeni (10 s)
  std::printf("== 6) odchod hrace pri spojovani (server mlci / spojeni visi) - stop() hned\n");
  {
    // a) server prijme TCP, ale na pozadavek nikdy neodpovi (handshake visi)
    int l = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a{}; a.sin_family = AF_INET; a.sin_addr.s_addr = htonl(INADDR_LOOPBACK); a.sin_port = 0;
    bind(l, (sockaddr *)&a, sizeof(a)); listen(l, 4);
    socklen_t al = sizeof(a); getsockname(l, (sockaddr *)&a, &al);
    OnlineKlient k;
    OnlineKlient::Nastaveni n;
    n.adresa = "http://127.0.0.1:" + std::to_string(ntohs(a.sin_port));
    n.cesta = std::string("/api/v1/online/zaloz?system=sega&otisk=") + OTISK;
    n.ja = ahoj("Rene");
    k.start(n);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    const bool spojuje = k.faze() == OnlineKlient::Faze::PRIPOJUJE;
    auto t0 = std::chrono::steady_clock::now();
    k.stop();
    const long ms = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    over(spojuje && ms < 500 && k.chyba() == "hra zrusena" && k.faze() == OnlineKlient::Faze::KONEC,
         "a) server mlci (handshake): stop() za " + std::to_string(ms) + " ms, stav '" + k.chyba() + "'");
    // b) zrus() pred spojenim (Java zavola stop hned po start) - spoj ani nezacne cekat
    k.start(n);
    t0 = std::chrono::steady_clock::now();
    k.stop();
    const long ms2 = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    over(ms2 < 500 && k.faze() == OnlineKlient::Faze::KONEC, "b) stop() hned po start(): " + std::to_string(ms2) + " ms, stav '" + k.chyba() + "'");
    close(l);
    // c) spojeni (TCP) visi: fronta naslouchajiciho socketu plna, dalsi SYN se zahazuje -> connect ceka
    int l2 = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a2{}; a2.sin_family = AF_INET; a2.sin_addr.s_addr = htonl(INADDR_LOOPBACK); a2.sin_port = 0;
    bind(l2, (sockaddr *)&a2, sizeof(a2)); listen(l2, 0);
    al = sizeof(a2); getsockname(l2, (sockaddr *)&a2, &al);
    std::vector<int> plni;
    for (int i = 0; i < 8; i++) {   // zaplnit frontu (nikdo neprijima)
      int c = socket(AF_INET, SOCK_STREAM, 0);
      fcntl(c, F_SETFL, fcntl(c, F_GETFL, 0) | O_NONBLOCK);
      connect(c, (sockaddr *)&a2, sizeof(a2));
      plni.push_back(c);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    n.adresa = "http://127.0.0.1:" + std::to_string(ntohs(a2.sin_port));
    k.start(n);
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    const bool spojuje2 = k.faze() == OnlineKlient::Faze::PRIPOJUJE;
    t0 = std::chrono::steady_clock::now();
    k.stop();
    const long ms3 = (long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0).count();
    over(ms3 < 500 && k.faze() == OnlineKlient::Faze::KONEC, std::string("c) TCP spojeni visi") + (spojuje2 ? "" : " (jadro spojeni prijalo - jako a)") +
                                                                ": stop() za " + std::to_string(ms3) + " ms, stav '" + k.chyba() + "'");
    for (int c : plni) close(c);
    close(l2);
    // d) po zruseni znovu normalne: start() zrus() zapomene
    n.adresa = g_adresa;
    k.start(n);
    const bool dostal = cekej([&] { return !k.kodMistnosti().empty(); }, 3000);   // nejdriv vysledek, pak text (poradi argumentu)
    over(dostal, "d) po zruseni dalsi start() normalne funguje: cislo hry " + k.kodMistnosti());
    k.stop();
  }

  std::printf("\nVYSLEDEK: %d OK, %d CHYB\n", g_ok, g_chyb);
  return g_chyb == 0 ? 0 : 1;
}
