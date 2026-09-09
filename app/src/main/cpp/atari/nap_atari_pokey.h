// nap_atari_pokey.h
// BUILD2SB52: POKEY zvuk - FAZE 1 z navrzeneho postupu (jen tolik, aby
// self-test v ROM vydal zvuk).
//
// Algoritmus je PRESNY PREKLAD z Reneho fungujici JS reference
// (emu_vbxe/index.html, funkce startAudio()/sp.onaudioprocess) - stejne
// polynomialni generatory (poly4/5/9/17), stejne bity AUDCTL, stejny
// vzorec periody kanalu, stejny DC-blocker na konci. Zadna vlastni teorie,
// jen preklad overeneho kodu do C++.
//
// JEDEN ROZDIL OD REFERENCE: JS reference generuje vzorky ASYNCHRONNE
// (samostatny WebAudio callback, ktery muze bezet driv nebo pozdeji nez
// CPU), a proto ma frontu udalosti (shadow queue "sh"/"q") pro vzorkove
// presne casovani zmen registru uprostred bufferu. Tenhle C++ kod
// generuje zvuk SYNCHRONNE - hned po tom, co CPU odbehne prislusny pocet
// cyklu (stejny vzor, jaky uz ma ANTIC/GTIA v nap_atari_video.h pres
// runFrame()) - takze fronta neni potreba, cte se proste AKTUALNI stav
// registru. Pro FAZI 1 (self-test, kde se AUDC/AUDF behem jednoho snimku
// obvykle nemeni) je to dostatecne presne - pro hudbu/efekty v pozdejsich
// fazich se muze ukazat, ze frontu bude nutne pridat, presne jak to ma
// reference.
//
// Co tu JE: 4 kanaly, cisty ton, poly4/5/9/17 sum, spojeni kanalu 1+2 a
//           3+4 (16bit), rychly hodinovy signal (1,79 MHz) pro kanal 1/3,
//           15kHz/64kHz zakladni takt, "volume only" (digi), DC-blocker.
// Co tu NENI: GTIA klaves-klik reproduktoru (CONSOL bit3), vysoce presne
//           vzorkove casovani zmen registru uprostred bufferu (viz vyse).
#pragma once
#include <cstdint>
#include <cstring>

namespace nap {

// BUILD2SB52: presne stejny generator jako poly4/5/9/17 v JS referenci -
// LFSR (linear feedback shift register), stejne odbocky (tap1,tap2).
struct PokeyPoly {
    uint8_t bits4[15];
    uint8_t bits5[31];
    uint8_t bits9[511];
    uint8_t bits17[131071];

    static void fill(uint8_t *out, int bitsN, int tap1, int tap2) {
        int len = (1 << bitsN) - 1;
        unsigned sr = (unsigned)len;
        for (int i = 0; i < len; i++) {
            out[i] = (uint8_t)(sr & 1);
            unsigned fb = ((sr >> tap1) ^ (sr >> tap2)) & 1;
            sr = (sr >> 1) | (fb << (bitsN - 1));
        }
    }
    PokeyPoly() {
        fill(bits4, 4, 3, 2);
        fill(bits5, 5, 4, 2);
        fill(bits9, 9, 8, 3);
        fill(bits17, 17, 16, 11);
    }
};

// Jeden staticky generator - tabulky se pocitaji jen jednou, sdilene
// pro celou appku (stejne jako by byly v JS jednou spocitane globalni
// promenne).
inline const PokeyPoly &pokeyPoly() {
    static PokeyPoly p;
    return p;
}

// BUILD2SB52: stav generatoru MEZI volanimi (pocitadla kanalu, pozice
// v poly tabulkach, DC-blocker pamet) - musi prezit napric vice volani
// genSamples(), jinak by kazdy snimek zacinal zvuk "od nuly" a tony by
// nemely spravnou frekvenci.
struct PokeyAudioState {
    double clock = 0.0;
    long cnt[4] = {0,0,0,0};
    int out[4] = {0,0,0,0};
    unsigned p4 = 0, p5 = 0, p9 = 0, p17 = 0;
    double dcx = 0.0, dcy = 0.0;
};

// BUILD2SB52: vygeneruje 'n' vzorku (mono, -1..1 jako float) z AKTUALNIHO
// stavu registru audf[4]/audc[4]/audctl. 'cyklu_na_vzorek' = kolik cyklu
// 1,79 MHz hodin POKEY pripada na jeden vystupni vzorek (viz CPS v JS
// referenci: 1773447/ac.sampleRate).
inline void pokeyGenSamples(const int audf[4], const int audc[4], int audctl,
                             PokeyAudioState &st, float *out, int n,
                             double cyklu_na_vzorek) {
    const PokeyPoly &poly = pokeyPoly();
    const int base15 = audctl & 1;
    const int j12 = audctl & 0x10;
    const int j34 = audctl & 0x08;

    for (int i = 0; i < n; i++) {
        double s = 0.0;
        st.clock += cyklu_na_vzorek;
        long steps = (long)st.clock;
        st.clock -= steps;
        st.p4  = (unsigned)((st.p4  + steps) % 15);
        st.p5  = (unsigned)((st.p5  + steps) % 31);
        st.p9  = (unsigned)((st.p9  + steps) % 511);
        st.p17 = (unsigned)((st.p17 + steps) % 131071);

        for (int ch = 0; ch < 4; ch++) {
            int ac8 = audc[ch];
            int vol = ac8 & 15;
            if (ac8 & 0x10) { s += vol / 60.0; continue; }   // volume only (digi)
            if (!vol) continue;

            long per;
            if (ch == 0 && (audctl & 0x40)) per = audf[0] + 4;
            else if (ch == 2 && (audctl & 0x20)) per = audf[2] + 4;
            else if (j12 && ch == 1) per = (long)(audf[0] + (audf[1] << 8) + 7) * (((audctl & 0x40) ? 1 : (base15 ? 114 : 28)));
            else if (j34 && ch == 3) per = (long)(audf[2] + (audf[3] << 8) + 7) * (((audctl & 0x20) ? 1 : (base15 ? 114 : 28)));
            else per = (long)(audf[ch] + 1) * (base15 ? 114 : 28);

            if (j12 && ch == 0) continue;
            if (j34 && ch == 2) continue;

            st.cnt[ch] += steps;
            while (st.cnt[ch] >= per) {
                st.cnt[ch] -= per;
                if (!(ac8 & 0x80) && !poly.bits5[st.p5]) continue;   // poly5 branka
                if (ac8 & 0x20) st.out[ch] ^= 1;                      // cisty ton
                else if (ac8 & 0x40) st.out[ch] = poly.bits4[st.p4];
                else st.out[ch] = (audctl & 0x80) ? poly.bits9[st.p9] : poly.bits17[st.p17];
            }
            s += st.out[ch] ? (vol / 60.0) : 0.0;
        }

        // DC-blocker (stejny vzorec jako reference: y[n] = x[n]-x[n-1]+0.995*y[n-1])
        double dcout = s - st.dcx + 0.995 * st.dcy;
        st.dcx = s; st.dcy = dcout;
        out[i] = (float)dcout;
    }
}

} // namespace nap
