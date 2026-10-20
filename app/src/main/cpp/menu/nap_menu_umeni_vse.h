// nap_menu_umeni_vse.h
// ====================================================================
//  B311: VSECHNA UMENI KARET A JEJICH VYROBA PODLE JMENA EFEKTU
//
//  Novy zivy efekt = nova trida (Umeni) a jeden radek ve vytvorUmeni().
//  Karta bez efektu (nebo s neznamym efektem) ukaze svuj obrazek.
// ====================================================================
#pragma once
#include "nap_menu_umeni.h"
#include "nap_menu_umeni_3d.h"
#include "nap_menu_umeni_kazeta.h"
#include "nap_menu_umeni_dzubox.h"
#include "nap_menu_umeni_web.h"
#include "nap_menu_umeni_help.h"

namespace napmenu {

inline std::unique_ptr<Umeni> vytvorUmeniEfekt(const std::string &efekt) {
  if (efekt == "3d:atari") return std::unique_ptr<Umeni>(new UmeniAtari());
  if (efekt == "3d:nap16") return std::unique_ptr<Umeni>(new UmeniKonzole(16));
  if (efekt == "3d:nap32") return std::unique_ptr<Umeni>(new UmeniKonzole(32));
  if (efekt == "fx:kazeta") return std::unique_ptr<Umeni>(new UmeniKazeta());
  if (efekt == "fx:dzubox") return std::unique_ptr<Umeni>(new UmeniDzubox());
  if (efekt == "fx:web") return std::unique_ptr<Umeni>(new UmeniWeb());
  if (efekt == "fx:help") return std::unique_ptr<Umeni>(new UmeniHelp());
  return nullptr;
}

// Umeni pro kartu: zivy efekt, kdyz ho C++ zna a jde pripravit; jinak obrazek;
// kdyz nejde ani ten, aspon pozadi karty (nazev dopise menu).
class UmeniJenPozadi : public Umeni {
 public:
  bool init(KontextUmeni &) override { return true; }
  void kresli(KontextUmeni &k, const RamecUmeni &r) override { k.pozadi(r, 0.36f, 0.40f, 0.f); }
  bool zive() const override { return false; }
  const char *nazev() const override { return "pozadi"; }
};

inline std::unique_ptr<Umeni> vytvorUmeni(KontextUmeni &k, const Karta &karta, std::string &log) {
  if (!karta.efekt.empty()) {
    std::unique_ptr<Umeni> u = vytvorUmeniEfekt(karta.efekt);
    if (!u) log += "karta " + karta.id + ": neznamy efekt '" + karta.efekt + "' - beru obrazek\n";
    else if (u->init(k)) return u;
    else log += "karta " + karta.id + ": efekt " + karta.efekt + " se nepodarilo pripravit - beru obrazek\n";
  }
  if (!karta.obrazek.empty()) {
    std::unique_ptr<Umeni> u(new UmeniObrazek(karta.obrazek));
    if (u->init(k)) return u;
    log += "karta " + karta.id + ": obrazek " + karta.obrazek + " nejde nacist\n";
  }
  return std::unique_ptr<Umeni>(new UmeniJenPozadi());
}

}  // namespace napmenu
