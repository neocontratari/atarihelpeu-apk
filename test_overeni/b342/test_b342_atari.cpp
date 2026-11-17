// B342: pristroj Atari 130XE (SKUTECNY nap_atari_device.h z appky) - tlacitko OTOCENI OBRAZOVKY.
// Na vysku pilulka "NA SIRKU" vpravo ve stavovem radku pod pristrojem, na sirku pilulka vlevo vedle
// kolecka. Stisk (pusteni nad tlacitkem) = servisni akce SVC_OTOC (12) pro Javu (NapObrazovka).
//   g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -o test_b342_atari test_b342_atari.cpp
//   ./test_b342_atari PRAC        -> obrazky PRAC/atari_*.ppm, vystup na stdout
#define STB_TRUETYPE_IMPLEMENTATION
#include "../../app/src/main/cpp/vendor/stb/stb_truetype.h"
#undef STB_TRUETYPE_IMPLEMENTATION
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
#include "nap_atari_device.h"
using namespace nap;

static int ok = 0, chyb = 0;
static void over(bool c, const std::string &co) {
  if (c) ok++; else chyb++;
  std::printf("%s%s\n", c ? "OK     " : "CHYBA  ", co.c_str());
}
static std::string f(const char *fmt, double a = 0, double b = 0, double c = 0, double d = 0, double e = 0) {
  char s[400]; std::snprintf(s, sizeof s, fmt, a, b, c, d, e); return s;
}
static void ulozPpm(dev::Device *d, const std::string &cesta, long long now) {
  std::vector<uint32_t> outb((size_t)d->W * d->H);
  d->present(outb.data(), d->W, now);
  FILE *fp = std::fopen(cesta.c_str(), "wb");
  if (!fp) return;
  std::fprintf(fp, "P6 %d %d 255\n", d->W, d->H);
  for (uint32_t c : outb) { std::fputc(c & 255, fp); std::fputc((c >> 8) & 255, fp); std::fputc((c >> 16) & 255, fp); }
  std::fclose(fp);
}
// tuknuti: dolu + nahoru na stejnem miste -> servisni akce
static int tukni(dev::Device *d, float x, float y, long long &now, int pid = 0) {
  dev::Ev ev[8]; int sa = 0;
  d->pointerDown(pid, x, y, now, ev, 8);
  now += 60;
  d->update(now); d->render(now);
  d->pointerUp(pid, x, y, now, ev, 8, &sa);
  now += 60;
  d->update(now); d->render(now);
  return sa;
}

int main(int argc, char **argv) {
  const std::string prac = argc > 1 ? argv[1] : ".";
  std::printf("B342 - pristroj Atari 130XE: tlacitko otoceni obrazovky (na vysku i na sirku)\n\n");
  Machine *m = new Machine(); AnticView *v = new AnticView();
  m->mem.os = NAP_OS_ROM; m->mem.bas = NAP_BASIC_ROM; m->view = v;
  m->reset(); m->consol = 7;
  for (int i = 0; i < 300; i++) m->runFrame();
  long long now = 100000;

  // ---------------- NA VYSKU (telefon 1080x2340, 1080x1920, uzky pruh TV 608x1080) ----------------
  struct Obr { int w, h; const char *jm; };
  const Obr vysky[] = {{1080, 2340, "telefon 1080x2340"}, {1080, 1920, "telefon 1080x1920 (16:9)"}, {608, 1080, "TV pruh 608x1080"},
                       {405, 720, "TV pruh 405x720"}};
  for (const Obr &o : vysky) {
    dev::Device *d = new dev::Device();
    d->layout(o.w, o.h);
    d->setAtariFrame(v->fb);
    d->update(now); d->render(now);
    dev::RRect r;
    const bool je = d->otocVyskaRR(r);
    over(je && !d->land, std::string(o.jm) + f(": pilulka NA SIRKU pod pristrojem vpravo (%.0f,%.0f %.0fx%.0f)", r.x0, r.y0, r.x1 - r.x0, r.y1 - r.y0));
    if (!je) { delete d; continue; }
    over(r.y0 >= d->Y(dev::DH) && r.y1 <= o.h && r.x1 <= d->X(dev::DW) + 1 && r.x0 >= d->X(0),
         std::string(o.jm) + ": cela pilulka na displeji, pod skrini pristroje, ne mimo jeho sirku");
    // zasah: stred pilulky = ID_OTOC; tesne nad skrini (servisni tlacitka, spodek skrine) = NE otoceni
    const float cx = (r.x0 + r.x1) * .5f, cy = (r.y0 + r.y1) * .5f;
    over(d->hitTest(cx, cy) == dev::ID_OTOC, std::string(o.jm) + ": dotyk na pilulku = otoceni");
    bool cizi = false;
    for (int i = 0; i < 8; i++) {                    // stredy servisnich tlacitek
      const float sx = d->X(dev::SVC_X0 + i * (dev::SVC_W + dev::SVC_GAP) + dev::SVC_W * .5f), sy = d->Y(dev::SVC_Y + dev::SVC_H * .5f);
      if (d->hitTest(sx, sy) != dev::ID_SVC0 + i) cizi = true;
    }
    over(!cizi, std::string(o.jm) + ": vsech 8 servisnich tlacitek (NET ... MENU) dal tam, kde byla");
    // zadny bod skrine pristroje (nad Y(DH)) neni otoceni
    bool presah = false;
    for (float y = d->Y(dev::DH) - 40.f; y < d->Y(dev::DH); y += 2.f)
      for (float x = d->X(0); x < d->X(dev::DW); x += 3.f) if (d->hitTest(x, y) == dev::ID_OTOC) presah = true;
    over(!presah, std::string(o.jm) + ": otoceni nezasahuje do skrine pristroje (posledni 40 px nad spodkem)");
    // tuknuti -> SVC_OTOC; tah mimo -> nic
    int sa = tukni(d, cx, cy, now);
    over(sa == dev::SVC_OTOC, std::string(o.jm) + f(": tuknuti -> servisni akce %.0f (SVC_OTOC = 12)", sa));
    {
      dev::Ev ev[8]; int sa2 = 0;
      d->pointerDown(1, cx, cy, now, ev, 8);
      d->pointerUp(1, cx, cy - 300.f, now + 50, ev, 8, &sa2);
      over(sa2 == 0, std::string(o.jm) + ": prst odjel z tlacitka -> nic (jako ostatni tlacitka)");
    }
    // MENU (8. servisni) porad vraci 8
    const float mx = d->X(dev::SVC_X0 + 7 * (dev::SVC_W + dev::SVC_GAP) + dev::SVC_W * .5f), my = d->Y(dev::SVC_Y + dev::SVC_H * .5f);
    sa = tukni(d, mx, my, now);
    over(sa == 8, std::string(o.jm) + f(": MENU porad akce %.0f", sa));
    if (o.w == 1080 && o.h == 2340) {
      // stisknuta pilulka (obrazek pro oko) + normalni
      ulozPpm(d, prac + "/atari_vyska_1080x2340.ppm", now);
      dev::Ev ev[8];
      d->pointerDown(2, cx, cy, now, ev, 8); d->update(now); d->render(now);
      ulozPpm(d, prac + "/atari_vyska_stisk.ppm", now);
      int sa3 = 0; d->pointerUp(2, cx, cy, now, ev, 8, &sa3);
    }
    if (o.w == 608) ulozPpm(d, prac + "/atari_pruh_608x1080.ppm", now);
    // vypnuty pristroj: otoceni jde (POWER off)
    {
      const float px = d->X(8 + 17), py = d->Y(393 + 25);
      tukni(d, px, py, now);
      const bool vyp = !d->power;
      int sa4 = tukni(d, cx, cy, now);
      over(vyp && sa4 == dev::SVC_OTOC, std::string(o.jm) + ": i pri vypnutem pristroji otoceni funguje");
      tukni(d, px, py, now);
    }
    delete d;
  }

  // ---------------- NA SIRKU (telefon 2340x1080, 1920x1080, TV 1280x720) ----------------
  const Obr sirky[] = {{2340, 1080, "telefon na sirku 2340x1080"}, {1920, 1080, "na sirku 1920x1080 (TV / telefon 16:9)"}, {1280, 720, "TV 1280x720"}};
  for (const Obr &o : sirky) {
    dev::Device *d = new dev::Device();
    d->layout(o.w, o.h);
    d->setAtariFrame(v->fb);
    d->update(now); d->render(now);
    over(d->land, std::string(o.jm) + ": rozlozeni na sirku");
    const dev::IRect g = d->gearRect(), r = d->otocRect();
    over(r.x1 <= g.x0 + 2 && r.x0 >= 0 && r.y0 >= 0 && r.y1 <= o.h,
         std::string(o.jm) + f(": otoceni vlevo vedle kolecka (otoceni x %.0f-%.0f, kolecko od %.0f)", r.x0, r.x1, g.x0));
    // zasah: prekryv se START/SELECT/OPTION a D-padem neni
    bool prekryv = false;
    for (int i = 0; i < dev::Device::C_N; i++) {
      const dev::RRect sh = d->ctlShape(i, d->cpx(i), d->cpy(i));
      if (!(sh.x1 < r.x0 || sh.x0 > r.x1 || sh.y1 < r.y0 || sh.y0 > r.y1)) prekryv = true;
    }
    over(!prekryv, std::string(o.jm) + ": otoceni neprekryva D-pad ani tlacitka (vychozi rozlozeni)");
    const float cx = d->otocCx, cy = d->otocCy;
    int sa = tukni(d, cx, cy, now);
    over(sa == dev::SVC_OTOC, std::string(o.jm) + f(": tuknuti na otoceni -> akce %.0f (SVC_OTOC)", sa));
    sa = tukni(d, d->gearCx, d->gearCy, now);
    over(sa == dev::SVC_CTRL, std::string(o.jm) + f(": kolecko porad menu ovladani (akce %.0f = SVC_CTRL)", sa));
    // B320: ovladani schovane (hraje gamepad) - otoceni porad jde
    d->nastavOvladac(2);
    sa = tukni(d, cx, cy, now);
    over(sa == dev::SVC_OTOC, std::string(o.jm) + ": schovane dotykove ovladani (gamepad) - otoceni porad funguje");
    d->nastavOvladac(0);
    if (o.w == 2340) {
      d->update(now); d->render(now);
      ulozPpm(d, prac + "/atari_sirka_2340x1080.ppm", now);
    }
    delete d;
  }

  // ---------------- otoceni displeje za chodu (telefon se otoci) ----------------
  {
    dev::Device *d = new dev::Device();
    d->layout(1080, 2340);
    d->setAtariFrame(v->fb);
    dev::Ev ev[8];
    dev::RRect r; d->otocVyskaRR(r);
    d->pointerDown(0, (r.x0 + r.x1) * .5f, (r.y0 + r.y1) * .5f, now, ev, 8);   // prst dole, displej se otoci
    d->layout(2340, 1080);
    int sa = 0;
    d->pointerUp(0, 100.f, 100.f, now + 40, ev, 8, &sa);
    over(sa == 0 && !d->otocDrzeno && d->otocPid < 0, "otoceni za chodu (prst na tlacitku): nic nezustane drzene, zadna akce navic");
    delete d;
  }

  std::printf("\nVYSLEDEK: %d OK, %d CHYBA\n", ok, chyb);
  return chyb ? 1 : 0;
}
