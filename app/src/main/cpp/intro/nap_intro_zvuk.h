// nap_intro_zvuk.h
// ====================================================================
//  ZVUK INTRA - syntéza po vzorcích, v C++.
//
//  Java si drzi AudioTrack a v pravidelnych davkach si sem chodi pro
//  vzorky (introNaplnZvuk). Zadny soubor, zadne nahravky - vsechno
//  se pocita.
//
//  Kazda konzole ma svuj hlas:
//    ATARI  ctvercove vlny a sum, jako POKEY. Klapani klaves je kratky
//           cvak reproduktorem (bit 3 registru CONSOL), ne pipnuti.
//    SEGA   dva operatory na sebe (FM), jak to dela YM2612 - jasnejsi,
//           kovovejsi zvuk.
//    PS1    mekke plochy a delsi doznivani, uz skoro hudba.
//
//  Zvuk Segy ("SEEEGAA") ani startovni zvuk Sony tu ZAMERNE nejsou -
//  to jsou jejich nahravky. Tohle je vlastni.
// ====================================================================
#pragma once
#include <cmath>
#include <cstring>
#include <atomic>
#include <vector>

namespace napintro {

const int VZORKOVANI = 44100;
const int HLASU = 24;

enum Tvar { T_CTVEREC, T_TROJUHELNIK, T_PILA, T_SINUS, T_SUM, T_FM };

struct Hlas {
    bool  zni = false;
    Tvar  tvar = T_CTVEREC;
    float f = 440.f;        // frekvence
    float a = 0.f;          // hlasitost
    float faze = 0.f;
    float t = 0.f;          // jak dlouho uz zni
    float delka = 0.f;
    float fmPomer = 2.0f;   // pro FM: pomer nosne a modulatoru
    float fmHloubka = 2.5f;
    float faze2 = 0.f;
    // Jak rychle ton doznivá. 1 = rovnomerne (basa, melodie),
    // 2 = rychle (bicí, klapani). Drive bylo 2 u vseho, takze
    // z kazdeho tonu byl slyset jen naraz a zbytek uz ne.
    float ostrost = 1.0f;
};

class Zvuk {
public:
    void start() {
        for (int i = 0; i < HLASU; i++) hlasy[i] = Hlas();
        cas = 0.0; taktDo = 0.0; takt = 0; sem = 0;
        bezi.store(true);
    }
    void stop() { bezi.store(false); }

    /** Prida hlas. Vraci false, kdyz je plno - to je v poradku, radsi
        vynechat ton nez skrastat. */
    bool pridej(Tvar tvar, float f, float delka, float a,
                float fmPomer = 2.0f, float fmHloubka = 2.5f, float ostrost = 1.0f) {
        for (int i = 0; i < HLASU; i++) {
            if (!hlasy[i].zni) {
                Hlas &h = hlasy[i];
                h.zni = true; h.tvar = tvar; h.f = f; h.a = a;
                h.faze = 0.f; h.faze2 = 0.f; h.t = 0.f; h.delka = delka;
                h.fmPomer = fmPomer; h.fmHloubka = fmHloubka; h.ostrost = ostrost;
                return true;
            }
        }
        return false;
    }

    /**
     * ZNELKA. Vlastni, ne napodobenina cizi.
     *  ktera=0  SEGA   siroky akord zdola nahoru, jasny a kovovy (FM)
     *  ktera=1  PS1    mekky stribrny nabeh, dlouhe doznivani
     */
    /**
     * Vlastni znelka NEBO nahravka ze ZARIZENI.
     * Kdyz si Rene da na telefon vlastni soubor, prehraje se ten;
     * jinak zahraje ta moje. Soubor se NEBALI do aplikace - stahne
     * se stejnou cestou jako hry z jeho webu.
     */
    void nastavNahravku(int ktera, const short *vzorky, int ramcu, int vzorkovani) {
        if (ktera < 0 || ktera > 1) return;
        Nahravka &n = nahravky[ktera];
        n.data.assign(vzorky, vzorky + (size_t)ramcu);
        n.ramcu = ramcu;
        n.krok = (vzorkovani > 0) ? (float)vzorkovani / (float)VZORKOVANI : 1.f;
        n.pozice = -1.f;
    }
    bool maNahravku(int ktera) const {
        return ktera >= 0 && ktera <= 1 && nahravky[ktera].ramcu > 0;
    }

    void znelka(int ktera) {
        if (ktera >= 0 && ktera <= 1 && nahravky[ktera].ramcu > 0) {
            nahravky[ktera].pozice = 0.f;      // spustit nahravku
            return;
        }
        if (ktera == 0) {
            // rozjezd zdola nahoru - pet tonu po sobe, FM at to ma hranu
            static const int noty[5] = { -12, -5, 0, 4, 7 };
            for (int i = 0; i < 5; i++)
                pridej(T_FM, pul(noty[i]), 1.6f - i*0.12f, 0.30f, 2.0f, 3.4f);
            pridej(T_SUM, 0, 0.30f, 0.22f, 2.f, 2.5f, 2.f);
            pridej(T_PILA, pul(-24), 1.9f, 0.28f);
        } else {
            // stribrny nabeh - cisty sinus a kvinta, dlouho dozniva
            static const int noty[4] = { 0, 7, 12, 19 };
            for (int i = 0; i < 4; i++)
                pridej(T_SINUS, pul(noty[i]), 2.4f - i*0.15f, 0.26f);
            pridej(T_TROJUHELNIK, pul(-24), 2.6f, 0.22f);
            pridej(T_SUM, 0, 0.55f, 0.10f);
        }
    }

    /** Klapnuti klavesy - kratky cvak, jako kdyz OS klapne reproduktorem. */
    void klapnuti(bool konecRadku) {
        if (konecRadku) { pridej(T_SUM, 0, 0.014f, 0.50f, 2.f, 2.5f, 2.f);
                          pridej(T_CTVEREC, 210, 0.032f, 0.24f, 2.f, 2.5f, 2.f); }
        else            { pridej(T_SUM, 0, 0.012f, 0.42f, 2.f, 2.5f, 2.f);
                          pridej(T_CTVEREC, 150 + (float)(nahoda()%40), 0.018f, 0.20f, 2.f, 2.5f, 2.f); }
    }

    /** Naplni davku vzorku. cas intra v ms rika, ktera cast hraje. */
    void naplni(short *ven, int pocet, int casMs) {
        if (!bezi.load()) { memset(ven, 0, sizeof(short) * pocet * 2); return; }
        hudba(casMs);
        const float dt = 1.0f / VZORKOVANI;
        for (int i = 0; i < pocet; i++) {
            float v = 0.f;
            // nahravka ze zarizeni, kdyz nejaka hraje
            for (int nn = 0; nn < 2; nn++) {
                Nahravka &n = nahravky[nn];
                if (n.pozice < 0.f || n.ramcu <= 0) continue;
                int idx = (int)n.pozice;
                if (idx >= n.ramcu) { n.pozice = -1.f; continue; }
                v += (n.data[idx] / 32768.f) * 1.6f;
                n.pozice += n.krok;
            }
            for (int k = 0; k < HLASU; k++) {
                Hlas &h = hlasy[k];
                if (!h.zni) continue;
                if (h.t >= h.delka) { h.zni = false; continue; }
                float obal = 1.f - (h.t / h.delka);
                if (h.ostrost > 1.5f) obal *= obal;          // bicí a klapani
                else                  obal = 0.25f + 0.75f*obal;  // drzi se dyl
                float vz = 0.f;
                switch (h.tvar) {
                    case T_SUM:
                        vz = (float)(nahoda() & 0xFFFF) / 32768.f - 1.f;
                        break;
                    case T_FM: {
                        h.faze2 += h.f * h.fmPomer * dt;
                        if (h.faze2 >= 1.f) h.faze2 -= 1.f;
                        float m = sinf(h.faze2 * 6.2831853f) * h.fmHloubka;
                        h.faze += h.f * dt;
                        if (h.faze >= 1.f) h.faze -= 1.f;
                        vz = sinf(h.faze * 6.2831853f + m);
                        break;
                    }
                    default:
                        h.faze += h.f * dt;
                        if (h.faze >= 1.f) h.faze -= 1.f;
                        if (h.tvar == T_CTVEREC)          vz = (h.faze < 0.5f) ? 1.f : -1.f;
                        else if (h.tvar == T_TROJUHELNIK) vz = 4.f * fabsf(h.faze - 0.5f) - 1.f;
                        else if (h.tvar == T_PILA)        vz = 2.f * h.faze - 1.f;
                        else                              vz = sinf(h.faze * 6.2831853f);
                        break;
                }
                v += vz * h.a * obal;
                h.t += dt;
            }
            v *= 0.45f;
            if (v > 1.f) v = 1.f;
            if (v < -1.f) v = -1.f;
            short s = (short)(v * 32000.f);
            ven[i*2] = s; ven[i*2+1] = s;
        }
    }

private:
    struct Nahravka {
        std::vector<short> data;   // mono
        int   ramcu = 0;
        float krok = 1.f;          // prevod vzorkovani
        float pozice = -1.f;       // -1 = nehraje
    };
    Nahravka nahravky[2];
    Hlas hlasy[HLASU];
    std::atomic<bool> bezi{false};
    double cas = 0.0, taktDo = 0.0;
    int takt = 0;
    unsigned sem = 1;

    unsigned nahoda() { sem = sem * 1664525u + 1013904223u; return sem >> 8; }
    static float pul(int p) { return 220.f * powf(2.f, p / 12.f); }

    /** Skladba. Kazda cast intra ma jiny hlas i jiny sled akordu. */
    void hudba(int casMs) {
        double ted = casMs / 1000.0;
        if (ted < taktDo) return;
        const double D = 0.155;               // doba
        taktDo = ted + D * 4;

        // Behem znelek a rozpadu skladba MLCI - jinak by se to pralo.
        // Hranice odpovidaji casove ose v nap_intro_gl.cpp.
        if ((casMs >= 19500 && casMs < 22500) ||     // znelka Sega
            (casMs >= 30500 && casMs < 33500) ||     // znelka PS1
            (casMs >= 41500 && casMs < 48200)) {     // chyba a rozpad
            return;
        }
        int cast;
        if (casMs < 19500)      cast = 0;     // Atari
        else if (casMs < 30500) cast = 1;     // Sega
        else if (casMs < 41500) cast = 2;     // PS1
        else                    cast = 3;     // finale na Atari

        static const int AKORDY[4][8] = {
            {  0,  0, -4, -4, -2, -2, -5, -5 },
            { -5, -5, -2, -2,  0,  0,  3,  3 },
            { -7, -7, -3, -3, -5, -5,  0,  0 },
            {  0, -2, -4, -5, -7, -5, -4, -2 },
        };
        static const int ARP[4][4] = {
            { 0, 3, 7, 10 }, { 0, 4, 7, 11 }, { 0, 3, 7, 12 }, { 0, 5, 9, 12 },
        };
        int akord = AKORDY[cast][takt % 8];

        // basa - u vsech, ale jinym tvarem
        Tvar tvarBasy = (cast == 0) ? T_TROJUHELNIK : (cast == 1 ? T_PILA : T_SINUS);
        pridej(tvarBasy, pul(akord - 24), (float)(D * 3.4), 0.34f);

        // arpeggio - ATARI ctverec, SEGA FM, PS1 mekky sinus
        Tvar tvarArp = (cast == 0) ? T_CTVEREC : (cast == 1 ? T_FM : T_SINUS);
        int kolik = (cast == 1) ? 6 : 4;
        for (int i = 0; i < kolik; i++) {
            int nota = akord + ARP[cast][i % 4] + (i >= 4 ? 12 : 0);
            pridej(tvarArp, pul(nota), (float)((D * 4 / kolik) * 0.8), 0.11f,
                   (cast == 1) ? 2.0f : 1.0f, (cast == 1) ? 3.2f : 0.f);
        }

        // melodie od druhe pulky kazdeho osmitakti
        if ((takt % 8) >= 4) {
            static const int LEAD[4][4] = {
                { 12, 10,  7, 10 }, { 15, 12, 19, 12 }, { 19, 17, 15, 12 }, { 24, 22, 19, 17 } };
            int l = LEAD[cast][takt % 4];
            Tvar tv = (cast == 0) ? T_PILA : (cast == 1 ? T_FM : T_TROJUHELNIK);
            pridej(tv, pul(akord + l), (float)(D * 1.7), 0.09f,
                   (cast == 1) ? 3.0f : 1.0f, (cast == 1) ? 2.2f : 0.f);
        }

        // bicí
        pridej(T_SUM, 0, 0.05f, 0.14f, 2.f, 2.5f, 2.f);
        if ((takt % 8) == 7) pridej(T_SUM, 0, 0.13f, 0.16f, 2.f, 2.5f, 2.f);
        takt++;
    }
};

} // namespace napintro
