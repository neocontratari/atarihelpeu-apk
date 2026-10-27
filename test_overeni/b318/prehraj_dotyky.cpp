// B318: dotyky zaznamenane na strance PS1 (Chromium, skutecne dotykove udalosti) prehraje
// do SKUTECNE logiky mysi (nap_ps1_mys.h) presne s jejich casy a mezi ne vlozi snimky
// jadra (60 Hz, hra kresli a cte kazdy snimek) - co by s nimi udelala hra. Pouziva test_b318_stranka.py.
//   vstup (stdin): prvni radek "rezim citlivost zrychleni", pak "akce id x y w h t_ms" (t z performance.now())
//   vystup: "posunX posunY klikuL klikuP snimkuL snimkuP vratilKlikL vratilKlikP nejvicZaSnimek"
#include "../../app/src/main/cpp/nap_ps1_mys.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

int main() {
  nap::Ps1Mys m;
  int rezim = 1, citlivost = 100, zrychleni = 1;
  std::cin >> rezim >> citlivost >> zrychleni;
  m.nastav(rezim, citlivost);
  m.nastavZrychleni(zrychleni != 0);
  long sx = 0, sy = 0;
  int kL = 0, kP = 0, sL = 0, sP = 0, vrL = 0, vrP = 0, maxKrok = 0;
  bool pL = false, pP = false;
  double snimekMs = -1;
  const unsigned port = (unsigned)(nap::Ps1Mys::portMysi(rezim) < 0 ? 0 : nap::Ps1Mys::portMysi(rezim));
  auto snimek = [&](double t) {
    m.snimek((long long)std::llround(t), 0);
    const int x = m.mysDotaz(port, nap::Ps1Mys::ID_X), y = m.mysDotaz(port, nap::Ps1Mys::ID_Y);
    sx += x;
    sy += y;
    if (std::abs(x) > maxKrok) maxKrok = std::abs(x);
    if (std::abs(y) > maxKrok) maxKrok = std::abs(y);
    const bool l = m.mysDotaz(port, nap::Ps1Mys::ID_LEVE) != 0, p = m.mysDotaz(port, nap::Ps1Mys::ID_PRAVE) != 0;
    sL += l; sP += p; kL += l && !pL; kP += p && !pP; pL = l; pP = p;
    m.poSnimku(true);
  };
  int akce, id;
  double x, y, w, h, t;
  while (std::cin >> akce >> id >> x >> y >> w >> h >> t) {
    if (snimekMs < 0) snimekMs = t;
    while (snimekMs + 1000.0 / 60 <= t) { snimekMs += 1000.0 / 60; snimek(snimekMs); }
    const int u = m.dotyk(akce, id, (float)x, (float)y, (float)w, (float)h, (long long)std::llround(t));
    vrL += (u & nap::Ps1Mys::KLIK_L) != 0;
    vrP += (u & nap::Ps1Mys::KLIK_P) != 0;
  }
  for (int i = 0; i < 90; i++) { snimekMs += 1000.0 / 60; snimek(snimekMs); }
  std::printf("%ld %ld %d %d %d %d %d %d %d\n", sx, sy, kL, kP, sL, sP, vrL, vrP, maxKrok);
  return 0;
}
