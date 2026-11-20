// app/src/main/cpp/online/nap_websocket.h
// =====================================================================================================
//  ONLINE HRANI (B357) - KLIENT WEBSOCKET (RFC 6455) PRO SPOJENI S ENGINEM N&P
// =====================================================================================================
//  Engine N&P (server Reneho) predava zpravy mezi dvema hraci. Spojeni jde pres STEJNY port jako vse
//  ostatni (http://server:port/api/v1/online ...) - WebSocket = jedno spojeni TCP, ktere zustane otevrene
//  a zpravy jdou obema smery hned (zadne nove spojeni pro kazdy stisk). Prihlaseni telefonu = stejne
//  hlavicky jako u ostatnich dotazu na engine (X-NaP-Zarizeni / Cas / Podpis - spocita je Java).
//  TCP_NODELAY: male zpravy (stisky) jdou hned, ne az se jich nasbira vic (Nagle).
//  Bez TLS (engine bezi na http). Ciste C++ (POSIX sockety) - test test_overeni/b357 proti nezavisle
//  implementaci WebSocketu (.NET WebSocket.CreateFromStream v mono).
// =====================================================================================================
#pragma once

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <random>
#include <string>
#include <vector>

namespace nap {
namespace online {

// --------------------------------------------------------------------------- SHA-1 (handshake, otisk hry)
struct Sha1 {
  uint32_t h[5] = {0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u};
  uint8_t blok[64];
  size_t vBloku = 0;
  uint64_t delka = 0;
  static uint32_t rol(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }
  void zpracuj(const uint8_t *b) {
    uint32_t w[80];
    for (int i = 0; i < 16; i++) w[i] = ((uint32_t)b[4 * i] << 24) | ((uint32_t)b[4 * i + 1] << 16) | ((uint32_t)b[4 * i + 2] << 8) | b[4 * i + 3];
    for (int i = 16; i < 80; i++) w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    uint32_t a = h[0], bb = h[1], c = h[2], d = h[3], e = h[4];
    for (int i = 0; i < 80; i++) {
      uint32_t f, k;
      if (i < 20) { f = (bb & c) | (~bb & d); k = 0x5A827999u; }
      else if (i < 40) { f = bb ^ c ^ d; k = 0x6ED9EBA1u; }
      else if (i < 60) { f = (bb & c) | (bb & d) | (c & d); k = 0x8F1BBCDCu; }
      else { f = bb ^ c ^ d; k = 0xCA62C1D6u; }
      const uint32_t t = rol(a, 5) + f + e + k + w[i];
      e = d; d = c; c = rol(bb, 30); bb = a; a = t;
    }
    h[0] += a; h[1] += bb; h[2] += c; h[3] += d; h[4] += e;
  }
  void pridej(const void *data, size_t n) {
    const uint8_t *p = (const uint8_t *)data;
    delka += n;
    while (n > 0) {
      const size_t k = std::min(n, (size_t)64 - vBloku);
      std::memcpy(blok + vBloku, p, k);
      vBloku += k; p += k; n -= k;
      if (vBloku == 64) { zpracuj(blok); vBloku = 0; }
    }
  }
  void konec(uint8_t out[20]) {
    const uint64_t bity = delka * 8;
    const uint8_t jedna = 0x80, nula = 0;
    pridej(&jedna, 1);
    while (vBloku != 56) pridej(&nula, 1);
    uint8_t l[8];
    for (int i = 0; i < 8; i++) l[i] = (uint8_t)(bity >> (56 - 8 * i));
    pridej(l, 8);
    for (int i = 0; i < 5; i++) for (int j = 0; j < 4; j++) out[4 * i + j] = (uint8_t)(h[i] >> (24 - 8 * j));
  }
};

inline std::string base64(const uint8_t *d, size_t n) {
  static const char *a = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string r;
  for (size_t i = 0; i < n; i += 3) {
    uint32_t v = (uint32_t)d[i] << 16;
    if (i + 1 < n) v |= (uint32_t)d[i + 1] << 8;
    if (i + 2 < n) v |= d[i + 2];
    r += a[(v >> 18) & 63];
    r += a[(v >> 12) & 63];
    r += i + 1 < n ? a[(v >> 6) & 63] : '=';
    r += i + 2 < n ? a[v & 63] : '=';
  }
  return r;
}

/** Sec-WebSocket-Accept pro dany klic (RFC 6455 4.2.2). */
inline std::string wsPrijeti(const std::string &klic) {
  const std::string s = klic + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  Sha1 h;
  h.pridej(s.data(), s.size());
  uint8_t o[20];
  h.konec(o);
  return base64(o, 20);
}

// =====================================================================================================
class WebSocket {
 public:
  WebSocket() {
    int p[2];
    if (pipe(p) == 0) {
      budik_[0] = p[0]; budik_[1] = p[1];
      fcntl(budik_[0], F_SETFL, O_NONBLOCK);
      fcntl(budik_[1], F_SETFL, O_NONBLOCK);
    }
  }
  ~WebSocket() {
    zavri();
    if (budik_[0] >= 0) close(budik_[0]);
    if (budik_[1] >= 0) close(budik_[1]);
  }
  WebSocket(const WebSocket &) = delete;
  WebSocket &operator=(const WebSocket &) = delete;

  /**
   * Pripoji se a udela handshake. adresa = "http://server:port" (nebo "ws://"), cesta = "/api/v1/online?...".
   * hlavicky = cele radky "Jmeno: hodnota" (podpis telefonu). Pri odmitnuti (401, 404 ...) je v 'chyba' kod
   * a telo odpovedi enginu (JSON s vysvetlenim).
   */
  bool spoj(const std::string &adresa, const std::string &cesta, const std::vector<std::string> &hlavicky,
            int timeoutMs, std::string &chyba, int *httpKod = nullptr) {
    zavri();
    if (budik_[0] >= 0) { char b[64]; while (read(budik_[0], b, sizeof(b)) > 0) {} }   // stare probuzeni pryc
    if (zruseno_) { chyba = "zruseno"; return false; }   // hrac uz odesel (zrus() pred spojenim)
    std::string host;
    int port = 80;
    if (!rozloz(adresa, host, port)) { chyba = "spatna adresa serveru: " + adresa; return false; }
    // --- TCP
    addrinfo rady{}, *vysl = nullptr;
    rady.ai_family = AF_UNSPEC;
    rady.ai_socktype = SOCK_STREAM;
    const std::string ps = std::to_string(port);
    const int g = getaddrinfo(host.c_str(), ps.c_str(), &rady, &vysl);
    if (g != 0 || !vysl) { chyba = std::string("server nejde najit (DNS): ") + gai_strerror(g); return false; }
    int s = -1;
    std::string posledni = "spojeni nejde";
    for (addrinfo *a = vysl; a; a = a->ai_next) {
      s = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
      if (s < 0) continue;
      fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
      int r = connect(s, a->ai_addr, a->ai_addrlen);
      if (r != 0 && errno == EINPROGRESS) {
        // po kouscich 100 ms - zrus() (hrac odesel) spojovani hned prerusi
        const auto konecSpojeni = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
        int chybaSpojeni = ETIMEDOUT;
        r = -1;
        while (!zruseno_) {
          const int zbyva = (int)std::chrono::duration_cast<std::chrono::milliseconds>(konecSpojeni - std::chrono::steady_clock::now()).count();
          if (zbyva <= 0) break;
          pollfd pf{s, POLLOUT, 0};
          const int p = poll(&pf, 1, std::min(zbyva, 100));
          if (p == 1) {
            int e = 0;
            socklen_t el = sizeof(e);
            getsockopt(s, SOL_SOCKET, SO_ERROR, &e, &el);
            if (e) chybaSpojeni = e; else r = 0;
            break;
          }
          if (p < 0 && errno != EINTR) { chybaSpojeni = errno; break; }
        }
        if (zruseno_) { close(s); freeaddrinfo(vysl); chyba = "zruseno"; return false; }
        if (r != 0) errno = chybaSpojeni;
      }
      if (r == 0) break;
      posledni = std::string("spojeni se serverem nejde: ") + std::strerror(errno);
      close(s);
      s = -1;
    }
    freeaddrinfo(vysl);
    if (s < 0) { chyba = posledni; return false; }
    fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) & ~O_NONBLOCK);
    int jedna = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &jedna, sizeof(jedna));
    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &jedna, sizeof(jedna));
    // --- handshake
    uint8_t k[16];
    nahodne(k, 16);
    const std::string klic = base64(k, 16);
    std::string pozadavek = "GET " + cesta + " HTTP/1.1\r\nHost: " + host + ":" + ps +
                            "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: " + klic +
                            "\r\nSec-WebSocket-Version: 13\r\n";
    for (const auto &h : hlavicky) pozadavek += h + "\r\n";
    pozadavek += "\r\n";
    if (!posliVse(s, (const uint8_t *)pozadavek.data(), pozadavek.size())) { close(s); chyba = "odeslani pozadavku nejde"; return false; }
    std::string odp;
    const auto konecCasu = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    size_t konecHlavicek = std::string::npos;
    while ((konecHlavicek = odp.find("\r\n\r\n")) == std::string::npos) {
      if (zruseno_) { close(s); chyba = "zruseno"; return false; }
      const int zbyva = (int)std::chrono::duration_cast<std::chrono::milliseconds>(konecCasu - std::chrono::steady_clock::now()).count();
      if (zbyva <= 0) { close(s); chyba = "server neodpovedel (timeout)"; return false; }
      pollfd pf{s, POLLIN, 0};
      const int p = poll(&pf, 1, std::min(zbyva, 100));
      if (p == 0 || (p < 0 && errno == EINTR)) continue;
      if (p < 0) { close(s); chyba = "server neodpovedel (poll)"; return false; }
      char b[2048];
      const ssize_t n = recv(s, b, sizeof(b), 0);
      if (n <= 0) { close(s); chyba = "server zavrel spojeni pri handshake"; return false; }
      odp.append(b, (size_t)n);
      if (odp.size() > 65536) { close(s); chyba = "prilis dlouha odpoved"; return false; }
    }
    const std::string hl = odp.substr(0, konecHlavicek);
    int kod = 0;
    if (hl.size() > 12 && hl.compare(0, 5, "HTTP/") == 0) kod = std::atoi(hl.c_str() + hl.find(' ') + 1);
    if (httpKod) *httpKod = kod;
    if (kod != 101) {
      // telo (JSON s chybou enginu) - co prislo hned s hlavickou (Content-Length nepocitam, staci zacatek)
      std::string telo = odp.substr(konecHlavicek + 4);
      pollfd pf{s, POLLIN, 0};
      if (telo.size() < 400 && poll(&pf, 1, 300) == 1) {
        char b[1024];
        const ssize_t n = recv(s, b, sizeof(b), 0);
        if (n > 0) telo.append(b, (size_t)n);
      }
      close(s);
      chyba = "engine odmitl online (HTTP " + std::to_string(kod) + ")" + (telo.empty() ? "" : ": " + telo.substr(0, 400));
      return false;
    }
    if (najdiHlavicku(hl, "sec-websocket-accept") != wsPrijeti(klic)) { close(s); chyba = "spatna odpoved serveru (Sec-WebSocket-Accept)"; return false; }
    {
      std::lock_guard<std::mutex> l(mPosli_);
      fd_ = s;
    }
    prijato_.assign(odp.begin() + (long)(konecHlavicek + 4), odp.end());   // data za hlavickou (prvni zpravy)
    zavreno_ = false;
    return true;
  }

  bool spojeno() const { return fd_ >= 0 && !zavreno_; }

  /** Posle zpravu (binarni 2 / text 1). Klient vzdy maskuje. Bezpecne z vic vlaken. */
  bool posli(const uint8_t *d, size_t n, uint8_t opcode = 2) {
    std::vector<uint8_t> f;
    f.reserve(n + 14);
    f.push_back((uint8_t)(0x80 | (opcode & 0x0F)));
    if (n < 126) f.push_back((uint8_t)(0x80 | n));
    else if (n < 65536) { f.push_back(0x80 | 126); f.push_back((uint8_t)(n >> 8)); f.push_back((uint8_t)n); }
    else { f.push_back(0x80 | 127); for (int i = 7; i >= 0; i--) f.push_back((uint8_t)((uint64_t)n >> (8 * i))); }
    uint8_t m[4];
    nahodne(m, 4);
    f.insert(f.end(), m, m + 4);
    const size_t z = f.size();
    f.resize(z + n);
    for (size_t i = 0; i < n; i++) f[z + i] = d[i] ^ m[i & 3];
    std::lock_guard<std::mutex> l(mPosli_);
    if (fd_ < 0 || zavreno_) return false;
    return posliVse(fd_, f.data(), f.size());
  }
  bool posli(const std::vector<uint8_t> &v, uint8_t opcode = 2) { return posli(v.data(), v.size(), opcode); }

  /**
   * Prijme jednu zpravu (binarni nebo textovou). Ping odpovi sam, pong ignoruje.
   * @return 1 zprava, 0 nic do timeoutu (nebo probuzeni), -1 spojeni skoncilo
   */
  int prijmi(std::vector<uint8_t> &zprava, int timeoutMs, uint8_t *opcode = nullptr) {
    const auto konecCasu = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs < 0 ? 0 : timeoutMs);
    while (true) {
      int r = rozeberRamec(zprava, opcode);
      if (r != 0) return r;
      if (fd_ < 0 || zavreno_) return -1;
      const int zbyva = (int)std::chrono::duration_cast<std::chrono::milliseconds>(konecCasu - std::chrono::steady_clock::now()).count();
      if (zbyva <= 0) return 0;
      pollfd pf[2] = {{fd_, POLLIN, 0}, {budik_[0], POLLIN, 0}};
      const int p = poll(pf, budik_[0] >= 0 ? 2 : 1, zbyva);
      if (p < 0) { if (errno == EINTR) continue; zavreno_ = true; return -1; }
      if (p == 0) return 0;
      if (budik_[0] >= 0 && (pf[1].revents & POLLIN)) {
        char b[64];
        while (read(budik_[0], b, sizeof(b)) > 0) {}
        if (!(pf[0].revents & (POLLIN | POLLHUP | POLLERR))) return 0;
      }
      if (pf[0].revents & (POLLIN | POLLHUP | POLLERR)) {
        uint8_t b[16384];
        const ssize_t n = recv(fd_, b, sizeof(b), 0);
        if (n <= 0) { zavreno_ = true; return -1; }
        prijato_.insert(prijato_.end(), b, b + n);
      }
    }
  }

  /** Jine vlakno: hrac odesel - prerusi spoj() (i pred nim) a probudi prijmi(). Plati do obnov(). */
  void zrus() { zruseno_ = true; probud(); }
  /** Pred novym spojenim (zadne jine vlakno s timhle spojenim nepracuje). */
  void obnov() { zruseno_ = false; }
  bool zruseno() const { return zruseno_; }

  /** Probudi vlakno cekajici v prijmi() (napr. kdyz je co poslat). */
  void probud() {
    if (budik_[1] >= 0) { const char c = 1; ssize_t w = write(budik_[1], &c, 1); (void)w; }
  }

  void zavri(bool slusne = true) {
    if (fd_ >= 0) probud();   // vlakno v prijmi() hned skonci
    std::lock_guard<std::mutex> l(mPosli_);
    if (fd_ < 0) return;
    if (slusne && !zavreno_) {
      const uint8_t f[6] = {0x88, 0x80, 0, 0, 0, 0};   // CLOSE s nulovou maskou, bez duvodu
      posliVse(fd_, f, sizeof(f));
    }
    shutdown(fd_, SHUT_RDWR);
    close(fd_);
    fd_ = -1;
    zavreno_ = true;
  }

  static bool rozloz(const std::string &adresa, std::string &host, int &port) {
    std::string a = adresa;
    port = 80;
    for (const char *p : {"http://", "ws://"}) if (a.compare(0, std::strlen(p), p) == 0) a = a.substr(std::strlen(p));
    if (a.compare(0, 8, "https://") == 0 || a.compare(0, 6, "wss://") == 0) return false;   // TLS neumim
    const size_t lom = a.find('/');
    if (lom != std::string::npos) a = a.substr(0, lom);
    if (a.empty()) return false;
    if (a[0] == '[') {   // IPv6 [::1]:8080
      const size_t z = a.find(']');
      if (z == std::string::npos) return false;
      host = a.substr(1, z - 1);
      if (z + 1 < a.size() && a[z + 1] == ':') port = std::atoi(a.c_str() + z + 2);
    } else {
      const size_t dv = a.rfind(':');
      if (dv != std::string::npos) { host = a.substr(0, dv); port = std::atoi(a.c_str() + dv + 1); }
      else host = a;
    }
    return !host.empty() && port > 0 && port < 65536;
  }

 private:
  static void nahodne(uint8_t *b, size_t n) {
    static std::mutex m;
    static std::mt19937 g{std::random_device{}()};
    std::lock_guard<std::mutex> l(m);
    for (size_t i = 0; i < n; i++) b[i] = (uint8_t)(g() & 0xFF);
  }

  static bool posliVse(int fd, const uint8_t *d, size_t n) {
    while (n > 0) {
      const ssize_t w = send(fd, d, n, MSG_NOSIGNAL);
      if (w < 0) { if (errno == EINTR) continue; return false; }
      d += w; n -= (size_t)w;
    }
    return true;
  }

  static std::string najdiHlavicku(const std::string &hl, const std::string &jmeno) {
    size_t i = 0;
    while (i < hl.size()) {
      size_t k = hl.find("\r\n", i);
      if (k == std::string::npos) k = hl.size();
      const std::string r = hl.substr(i, k - i);
      const size_t dv = r.find(':');
      if (dv != std::string::npos) {
        std::string j = r.substr(0, dv);
        for (char &c : j) c = (char)std::tolower((unsigned char)c);
        if (j == jmeno) {
          size_t z = dv + 1;
          while (z < r.size() && (r[z] == ' ' || r[z] == '\t')) z++;
          size_t e = r.size();
          while (e > z && (r[e - 1] == ' ' || r[e - 1] == '\t')) e--;
          return r.substr(z, e - z);
        }
      }
      i = k + 2;
    }
    return "";
  }

  /** Vybere z prijato_ jeden cely ramec / zpravu. 1 zprava, 0 malo dat, -1 konec (CLOSE). */
  int rozeberRamec(std::vector<uint8_t> &zprava, uint8_t *opcode) {
    while (true) {
      if (prijato_.size() < 2) return 0;
      const uint8_t b0 = prijato_[0], b1 = prijato_[1];
      const bool fin = (b0 & 0x80) != 0, maska = (b1 & 0x80) != 0;
      const uint8_t op = b0 & 0x0F;
      uint64_t n = b1 & 0x7F;
      size_t h = 2;
      if (n == 126) { if (prijato_.size() < 4) return 0; n = ((uint64_t)prijato_[2] << 8) | prijato_[3]; h = 4; }
      else if (n == 127) { if (prijato_.size() < 10) return 0; n = 0; for (int i = 0; i < 8; i++) n = (n << 8) | prijato_[2 + i]; h = 10; }
      if (n > (64u << 20)) { zavreno_ = true; return -1; }   // nesmysl (>64 MB)
      uint8_t m[4] = {0, 0, 0, 0};
      if (maska) { if (prijato_.size() < h + 4) return 0; std::memcpy(m, &prijato_[h], 4); h += 4; }
      if (prijato_.size() < h + n) return 0;
      std::vector<uint8_t> data(prijato_.begin() + (long)h, prijato_.begin() + (long)(h + n));
      if (maska) for (size_t i = 0; i < data.size(); i++) data[i] ^= m[i & 3];
      prijato_.erase(prijato_.begin(), prijato_.begin() + (long)(h + n));
      if (op == 0x8) { zavreno_ = true; return -1; }                       // CLOSE
      if (op == 0x9) { posli(data.data(), data.size(), 0xA); continue; }   // PING -> PONG
      if (op == 0xA) continue;                                             // PONG
      if (op == 0x0) {                                                     // pokracovani zpravy
        cast_.insert(cast_.end(), data.begin(), data.end());
        if (!fin) continue;
        zprava.swap(cast_);
        cast_.clear();
        if (opcode) *opcode = castOp_;
        return 1;
      }
      if (!fin) { cast_ = data; castOp_ = op; continue; }                  // zacatek delene zpravy
      zprava.swap(data);
      if (opcode) *opcode = op;
      return 1;
    }
  }

  int fd_ = -1;
  volatile bool zavreno_ = true;
  std::atomic<bool> zruseno_{false};
  int budik_[2] = {-1, -1};
  std::mutex mPosli_;
  std::vector<uint8_t> prijato_, cast_;
  uint8_t castOp_ = 2;
};

}  // namespace online
}  // namespace nap
