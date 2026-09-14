// nap_atari_machine.h
// BUILD2SA17: stroj - hardwarove registry a snimkova smycka.
//
// Ucel: rozbehnout SELF-TEST Z ROM ATARI ($5000-$57FF). Je to Reneho vlastni
// ROM a jeho vlastni diagnostika - neni to nic, co bych si vymyslel.
// Self-test si sam nastavi display list, obrazovku i barvy a sam si otestuje
// pamet. Kdyz to nase jadro rozbehne a ukaze totez co skutecne Atari,
// je to dukaz. Kdyz ne, je videt PRESNE kde to skonci.
//
// Co uz tu je: ANTIC (DMACTL, DLIST, CHBASE, NMIEN/NMIST, VCOUNT, WSYNC),
//              GTIA (barvy, CONSOL, TRIG, PAL), PIA, POKEY jen tolik,
//              aby se ROM nezasekla (RANDOM, SKSTAT, IRQST, KBCODE).
// Co tu NENI: zvuk, hraci a strely, kolize, skrolovani, ostatni graficke
//              rezimy. Nebudu predstirat opak.
#pragma once
#include <cstdint>
#include <cstring>
#include "nap_atari_cpu.h"
#include "nap_atari_mem.h"
#include "nap_atari_video.h"
#include "nap_atari_pokey.h"

namespace nap {

struct Machine {
  AtariMem mem;
  Cpu6502  cpu;

  // ANTIC
  int dmactl = 0, chactl = 0, dlistL = 0, dlistH = 0;
  int hscrol = 0, vscrol = 0, pmbase = 0, chbase = 0;
  int nmien = 0, nmist = 0x1F;
  int line = 0;            // scanline 0..311 (PAL)
  long long frame = 0;

  // GTIA
  int hposp[4] = {0,0,0,0}, hposm[4] = {0,0,0,0};
  int sizep[4] = {0,0,0,0}, sizem = 0;
  int grafp[4] = {0,0,0,0}, grafm = 0;
  int colpm[4] = {0,0,0,0};
  int colpf[4] = {0,0,0,0};
  int colbk = 0, prior = 0, gractl = 0, vdelay = 0;
  int consol = 7;          // 0 = stisknuto; 7 = nic
  int trig[4] = {1,1,1,1};

  // POKEY - jen tolik, aby ROM nezustala viset
  int skctl = 0, irqen = 0, kbcode = 0;
  int irqst = 0xFF;        // 0 v bitu = preruseni CEKA
  int serout = 0;
  // BUILD2SB84: Rene - "chybi zaverecni chrceni CSAVE - jen uvodni
  // piskot je slyset." PRESNE OVERENO v JS referenci (jediny zdroj
  // pravdy pro logiku): "if(pokey.outBusy>0 && --pokey.outBusy===0)
  // pokeyRaise(0x10); if(pokey.shiftBusy>0 && --pokey.shiftBusy===0)
  // pokeyRaise(0x08);" - DVA NEZAVISLE citace, KAZDY snizeny o 1
  // KAZDY radek obrazovky (scanline), NE 2 spolecne kroky jak jsem
  // mel puvodne. Moje puvodni "serOdpocet=2" bylo ~15x AZ 140x
  // rychlejsi nez skutecny prenos - proto se cely blok dat (256+
  // bajtu) odeslal za 0.3s misto realistickych ~4.7s pri 600 baudech,
  // a zvuk pak vypadal jako jeden nerozeznatelny impuls, ne jako
  // "chrceni" rozlozene v case.
  int outBusy = 0;   // scanline do "vystupni registr prazdny, dej dalsi" (bit4/0x10)
  int shiftBusy = 0; // scanline do "cely prenos bajtu dokoncen" (bit3/0x08)

  // BUILD2SB85: Rene - "reset/power/self-test presne jak ma, CSAVE
  // piska spravne, ale na konci pisknuti chybi to puvodni chrapteni -
  // ten datovy zvuk." PRESNE OVERENO (ne odhad): behem cele 42-
  // vterinove tonove faze v realnem logu appka NIKDY nemeni AUDF,
  // Timer4 preruseni je po celou dobu VYPNUTE (0 vyvolani) - zadny
  // softwarovy mechanismus v ROM nepřepisuje AUDF bit po bitu.
  // Overeno i v JS referenci - genericka POKEY syntéza (AUDF/AUDC/
  // AUDCTL), ZADNA zminka o SKCTL/dvoutonovem rezimu/sériovem bitu
  // v syntéze zvuku vubec. Zavěr: skutecny POKEY hardware v
  // "dvoutonovem" rezimu (SKCTL bit3, primo overeno jako rozlisujici
  // bit CSAVE/CLOAD v B274) dela vyber mezi dvema frekvencemi SAM,
  // v kremiku, podle AKTUALNIHO BITU v posuvnem registru sériovych
  // dat - software jen NALOZI bajt (STA SEROUT) a hardware uz sam
  // "odsype" bity, prepinaje frekvenci. Tenhle mechanismus v me
  // emulaci CHYBEL UPLNE - proto zvuk zustaval konstantni. Nize
  // pridany stav simuluje presne tohle: 10-bitovy ramec (start+8
  // datovych bitu LSB prvni+stop) pro kazdy zapsany bajt, s
  // casovanim podle 600 baudu (~2956 CPU cyklu/bit, presne cyklu_na_
  // vzorek konstanta CPS/600 jako v JS referenci pro CTENÍ pasky).
  int serRamec = 0;              // 10 bitu: bit0=start(0), 1-8=data LSB, 9=stop(1)
  long long serRamecStart = 0;   // CPU cyklus, kdy byl ramec nalozen
  uint32_t rngState = 0x2A5C1D7B;

  // BUILD2SB52: POKEY zvuk (FAZE 1) - viz nap_atari_pokey.h pro
  // generovani vzorku. Tady jen registry, presne jak je cte/zapisuje
  // ROM (AUDF1,AUDC1,AUDF2,AUDC2,AUDF3,AUDC3,AUDF4,AUDC4,AUDCTL).
  int audf[4] = {0,0,0,0};
  int audc[4] = {0,0,0,0};
  int audctl = 0;
  PokeyAudioState pokeyAudio;
  // BUILD2SB65: Rene - "csave neni kazetovy port... to bylo vyreseno
  // v jave emu atari, udelej to presne podle atari jadra." Nalezeno:
  // CSAVE po druhem RETURN vstoupi do smycky, ktera ceka na hodnotu
  // nastavovanou PRERUSENIM OD POKEY CASOVACE 1/2 (presne to, co se
  // pouziva pro casovani kazetovych bitu I BEZ pripojeneho kazetaku -
  // Rene mel pravdu, ze to s fyzickym zarizenim nema nic spolecneho,
  // je to cisty vnitrni casovac cipu). Predtim appka mela POKEY jen
  // "tak akorat, aby ROM nezustala viset" - casovace 1/2/4 vubec
  // nebyly. Ted presny preklad z Reneho fungujici JS reference
  // (timerPeriod/timersReload/timersTick).
  long timer1Cyc = 0, timer2Cyc = 0, timer4Cyc = 0;
  // BUILD2SB56: Rene - "pri bootovani ma atari svuj specificky zvuk,
  // klik pri startu. V self-testu je to lepsi nez v Java Atari, ale
  // ma to podzvuk." Prvni cast: GTIA "klik reproduktoru" (CONSOL bit3,
  // $D01F) - OS na to piše KAZDY SNIMEK behem bootu (viz komentar u
  // hwWrite nize) a presne TOHLE je ten chybejici "boot zvuk". Stejny
  // mechanismus jako v JS referenci (M.onSpeaker) - kratky (~4ms)
  // napet'ovy skok, ne skutecny tón.
  int gtiaSpeakerBit = 0;      // aktualni stav bitu (0/1), zapisuje ho hwWrite
  int gtiaSpeakerVidenaAudioGen = 0; // co naposledy videl genAudio() - pro detekci ZMENY
  // BUILD2SB60: Rene - "bootovaci zvuk neni spravny." Puvodni logovani
  // (regsNative) ukazovalo jen AUDF/AUDC/AUDCTL - ale klik jede pres
  // UPLNE JINA pole (spkLevel/spkDecay v PokeyAudioState), takze z
  // logu neslo poznat, jestli se kliky VUBEC spoustely. Pocitadlo
  // pro viditelnost - ne pro zvuk samotny.
  long long gtiaKlikPocitadlo = 0;

  // BUILD2SB74: Rene - "u atari nestrim zadny fake, nebud liny a nehadej."
  // Dukladnym trasovanim (ne hadanim, ani domnenkou) zjisteno: appka
  // SPRAVNE prochazi "pipaci" smyckou v ROM ($FE00-$FE36 - rychle
  // opakovane prepinani CONSOL bit3, presny stejny mechanismus jako
  // klik pri bootu, jen mnohem castejsi) hned po prvnim RETURN - jadro
  // NENI rozbite. Skutecny problem: genAudio() se vola JEDNOU ZA
  // SNIMEK a kontroluje jen "je bit ted jiny nez naposledy?" - jenze
  // tahle ROM smycka prepina bit MNOHOKRAT BEHEM JEDNOHO SNIMKU (tón,
  // ne jednotlivy klik). Vzorkovani jen na konci snimku ztrati VSECHNY
  // mezilehle prechody. Oprava: zaznamenat KAZDY prechod s presnym
  // CPU cyklem primo pri zapisu, genAudio() je pak zpracuje vsechny na
  // spravnych pozicich v ramci bufferu.
  static const int MAX_SPEAKER_PRECHODU = 96;
  struct SpeakerPrechod { long long cyklus; int novaHodnota; };
  SpeakerPrechod speakerPrechody[MAX_SPEAKER_PRECHODU];
  int pocetSpeakerPrechodu = 0;

  const uint8_t *osRom = nullptr;
  const uint8_t *basRom = nullptr;

  // Prubezne cteni display listu - ANTIC ho nezpracuje najednou, ale
  // postupne, jak sjizdi obrazovku. Bez toho by DLI nemely kdy zabrat.
  AnticView *view = nullptr;

  // SLEDOVANI PAPRSKU
  // Program meni registry GTIA UPROSTRED radky (typicky hned po WSYNC).
  // Kdyz se pro cely radek vezme jedna hodnota, multiplexovany kernel se
  // rozsype - u Decathlonu se z atletu stanou svisle bloky.
  // Proto se kazdy zapis zaznamena i s cyklem, ve kterem prisel, a pri
  // kresleni se pro kazdy bod pouzije to, co v tu chvili platilo.
  long long lineCyc0 = 0;
  static const int SEG_MAX = 192;
  int segN = 0;
  int segCyk[SEG_MAX], segReg[SEG_MAX], segHod[SEG_MAX];
  // 0x00-0x1B registry GTIA, 0x20-0x2F registry ANTIC
  int segZac[0x30];                       // stav na ZACATKU radky
  int dlPc = 0, dlScreen = -1, dlMode = 0, dlZbyva = 0, dlRadek = 0, dlKroku = 0;
  bool dlDli = false;
  bool dlKonec = false;
  bool dlHskrol = false, dlVskrol = false;
  bool stopa=false; int stopaMode[240]={0}, stopaScr[240]={0}, stopaRad[240]={0};

  Machine()
    : cpu([this](int a){ return this->read(a); },
          [this](int a,int v){ this->write(a,v); }) {}

  inline int rnd() {
    rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5;
    return (int)(rngState >> 16) & 0xFF;
  }

  int read(int a) {
    a &= 0xFFFF;
    if (a >= 0xD000 && a < 0xD800) return hwRead(a);
    return mem.cpuRead(a);
  }
  void write(int a, int v) {
    a &= 0xFFFF; v &= 0xFF;
    if (a >= 0xD000 && a < 0xD800) { hwWrite(a, v); return; }
    mem.cpuWrite(a, v);
  }

  int hwRead(int a) {
    const int page = a & 0xFF00;
    if (page == 0xD000) {                       // GTIA
      const int r = a & 0x1F;
      if (r == 0x10) return trig[0];            // TRIG0
      if (r == 0x11) return trig[1];
      if (r == 0x12) return trig[2];
      if (r == 0x13) return trig[3];
      if (r == 0x14) return 0x01;               // PAL: 1 = PAL stroj
      if (r == 0x1F) return consol & 7;         // CONSOL
      return 0x0F;                              // kolize - zatim nic
    }
    if (page == 0xD200) {                       // POKEY
      const int r = a & 0x0F;
      if (r == 0x09) return kbcode;             // KBCODE
      if (r == 0x0A) return rnd();              // RANDOM
      if (r == 0x0E) return irqst;              // IRQST
      if (r == 0x0F) {                          // SKSTAT
        // BUILD2SB71: Rene - "to atari a pokey a logika ti evidentne
        // nejde... podivej se do kodu atari emu 130xe vbxe JAVA - tam
        // je navod." MEL PRAVDU a ja jsem ho neposlechl vcas - moje
        // predchozi "sum" oprava (BUILD2SB69) byla SPATNE, a presne
        // takovou chybu uz nekdo v minulosti zkusil a ZAVRHL primo v
        // JS referenci: "Zasada BUILD2BR - zadny fake CLOAD, zadny RAM
        // inject... vlastni CSAVE nepousti datovy chaos do SKSTAT."
        // SKUTECNA reference (index.html, cteni $D20F) ukazuje: bit4
        // se meni JEN kdyz existuje SKUTECNY kazetovy tón/PCM signal A
        // bezi motor ("if((cassTone||cassPcm||cassRecordLead) &&
        // cassMotor)") - jinak zustava na VYCHOZI hodnote (1, od
        // v=0xFF) - presne to, co appka delala PRED mou chybnou
        // opravou! Bez nahrane kazety ROM smycka SPRAVNE ceka a pak
        // spadne do zalozniho casoveho limitu - to NENI bug, to je
        // spravne chovani pro "CSAVE bez pripojeneho kazetaku". Vraceno
        // zpet na puvodni, referenci odpovidajici hodnotu.
        return 0xFF;
      }
      return 0xFF;
    }
    if (page == 0xD300) {                       // PIA
      const int r = a & 0x03;
      if (r == 0) return (mem.pia.ctlA & 4) ? 0xFF : mem.pia.ddrA;
      if (r == 1) return (mem.pia.ctlB & 4) ? mem.portB() : mem.pia.ddrB;
      if (r == 2) return mem.pia.ctlA;
      return mem.pia.ctlB;
    }
    if (page == 0xD400) {                       // ANTIC
      const int r = a & 0x0F;
      if (r == 0x0B) return (line >> 1) & 0xFF; // VCOUNT
      if (r == 0x0F) return nmist;              // NMIST
      return 0xFF;
    }
    return 0xFF;
  }

  void hwWrite(int a, int v) {
    const int page = a & 0xFF00;
    if (page == 0xD000) {                       // GTIA
      const int r = a & 0x1F;
      if (r < 0x1C && segN < SEG_MAX) {         // poznamenat CYKLUS zapisu
        segCyk[segN] = (int)(cpu.c.cycles - lineCyc0);
        segReg[segN] = r; segHod[segN] = v & 0xFF; segN++;
      }
      if (r <= 0x03) { hposp[r] = v; return; }
      if (r <= 0x07) { hposm[r - 4] = v; return; }
      if (r <= 0x0B) { sizep[r - 8] = v; return; }
      if (r == 0x0C) { sizem = v; return; }
      if (r >= 0x0D && r <= 0x10) { grafp[r - 0x0D] = v; return; }
      if (r == 0x11) { grafm = v; return; }
      if (r >= 0x12 && r <= 0x15) { colpm[r - 0x12] = v; return; }
      if (r >= 0x16 && r <= 0x19) { colpf[r - 0x16] = v; return; }
      if (r == 0x1A) { colbk = v; return; }
      if (r == 0x1B) { prior = v; return; }
      if (r == 0x1C) { vdelay = v; return; }
      if (r == 0x1D) { gractl = v & 7; return; }
      if (r == 0x1F) {
        // POZOR: zapis do CONSOL ovlada REPRODUKTOR a vystupni zapadku.
        // NESMI mazat stav tlacitek - OS sem pise pri KAZDEM snimku
        // (pro klapnuti reproduktoru), takze by tim smazal kazdy stisk
        // START/SELECT/OPTION driv, nez si ho program stihne precist.
        //
        // BUILD2SB56: presne TADY vznika chybejici "boot zvuk" - bit3
        // je vystup na reproduktor.
        // BUILD2SB74: zaznamenat KAZDY prechod s cyklem, ne jen ulozit
        // posledni stav - viz komentar u pole speakerPrechody vyse.
        int novyBit = (v >> 3) & 1;
        if (novyBit != gtiaSpeakerBit) {
          if (pocetSpeakerPrechodu < MAX_SPEAKER_PRECHODU) {
            speakerPrechody[pocetSpeakerPrechodu].cyklus = cpu.c.cycles;
            speakerPrechody[pocetSpeakerPrechodu].novaHodnota = novyBit;
            pocetSpeakerPrechodu++;
          }
          gtiaSpeakerBit = novyBit;
        }
        return;
      }
      return;
    }
    if (page == 0xD200) {                       // POKEY
      const int r = a & 0x0F;
      // BUILD2SB52: AUDF1,AUDC1,AUDF2,AUDC2,AUDF3,AUDC3,AUDF4,AUDC4 -
      // presne stejne rozlozeni jako v JS referenci ("if(p<=0x07){
      // if(p&1) audc[p>>1]=v; else audf[p>>1]=v;}").
      if (r <= 0x07) {
        if (r & 1) audc[r >> 1] = v & 0xFF;
        else       audf[r >> 1] = v & 0xFF;
        return;
      }
      if (r == 0x08) { audctl = v & 0xFF; return; }   // AUDCTL
      if (r == 0x09) {                                // STIMER - viz JS "timersReload()"
        // BUILD2SB65: STIMER ted opravdu ZNOVUNABIJI casovace 1/2/4
        // (drive to komentar sliboval, ale kod to nedelal - presne to
        // zpusobovalo, ze CSAVE po druhem RETURN cekala na preruseni,
        // ktere nikdy neprislo). STIMER navic podle reference vynuluje
        // i vystupni citadla zvukovych kanalu.
        cnt0Reset();
        timersReload();
        return;
      }
      if (r == 0x0D) {                          // SEROUT
        // BUILD2SB86: Rene - "piskani ~20s, datovy tok ~4-5s - to na
        // realnem Atari (i s Altirrou) takhle zni." PRESNE OVERENO:
        // 17.84s cekani (viz $037C/892 snimku nalezene v B274-85)
        // OPRAVDU odpovida realnym ~20s piskotu - TOHLE JE SPRAVNE.
        // Ale samotny DATOVY TOK byl AZ 10x kratsi nez ma byt: puvodni
        // "outBusy=10, shiftBusy=30" (z JS reference - hodnoty tam
        // zjevne predstavuji jen "kdy je vystupni registr pripraven
        // na dalsi bajt", NE "za jak dlouho doopravdy dobehne cele
        // vysilani na drate") delaly, ze appka prijala DALSI bajt
        // DRIV, nez stihl predchozi bajt DOHRAT svych 10 bitu zvuku -
        // kazdy novy zapis PREPSAL serRamecStart uprostred prehravani
        // predchoziho bajtu, takze se cely 260+ bajtovy blok zmackl
        // do necelé vteřiny místo spravnych ~4-5s. OPRAVA: outBusy a
        // shiftBusy ted odpovidaji SKUTECNE dobe potrebne na odeslani
        // 10 bitu pri 600 baudech (10/600s = 16.67ms = presne
        // cyklu_na_bit*10/114 scanline, ~260 scanline) - appka tak
        // dostane "pripraveno na dalsi" AZ KDYZ predchozi bajt
        // doopravdy cely dohral, presne jako na skutecnem hardwaru.
        serout = v;
        const int SCANLINE_NA_BAJT = (int)((1773447.0/600.0)*10.0/114.0 + 0.5); // ~260
        // BUILD2SB86 KRITICKA POJISTKA (znovuobjeveny puvodni problem
        // z B268 komentare vyse): outBusy a shiftBusy NESMI byt
        // STEJNE - kdyz obe preruseni (bit4 "pripraven na dalsi" a
        // bit3 "cely prenos hotov") vystrely na STEJNE scanline, SIO
        // rutina si mysli, ze je hotovo hned po prvnim bajtu a
        // ZUSTANE VISET (presne overeno - obrazovka se zasekla,
        // PC uvizl v $EAA0 oblasti). outBusy zustava o kousek KRATSI
        // (jako by dvojite-bufferovany UART prijal dalsi bajt TESNE
        // pred dokoncenim stop bitu predchoziho) - shiftBusy o 2
        // scanline delsi, at nikdy nevystrely soucasne.
        outBusy = SCANLINE_NA_BAJT - 2;
        shiftBusy = SCANLINE_NA_BAJT;
        // BUILD2SB85: nalozit 10-bitovy sériovy ramec pro dvouton
        // (start=0, 8 datovych bitu LSB prvni, stop=1) - genAudio()
        // nize podle tohohle vybira, kterou ze dvou frekvenci prave
        // hrat.
        serRamec = 0x200 | (v << 1); // bit0=0(start), bity1-8=data, bit9 uz je 1 z 0x200
        serRamecStart = cpu.c.cycles;
        return;
      }
      if (r == 0x0E) {                          // IRQEN
        // BUILD2SB67: Rene - "atari nejde nabootovat, modra obrazovka
        // se ctverečkem, porad se opakujici zvuk." NALEZENO: predchozi
        // B258 "instant ready" oprava (kdyz je serialovy port v klidu
        // a IRQEN prave povoli bit 0x10/0x08, hned ohlasit pripraveno)
        // byla PRILIS SIROKA - OS ji spousti i behem UPLNE NORMALNIHO
        // bootu (ne jen behem CSAVE), coz appku odklonilo z normalni
        // klidove smycky ($F302-$F310) do jineho, spatneho stavu se
        // stale hrajicim tónem. OVERENO srovnavacim testem: SAMOTNE
        // casovace (nize, timersTick) CSAVE odblokuji stejne spolehlive
        // (do par tisic snimku se vrati do normalni klidove smycky,
        // ticho) BEZ POTREBY tohohle rizikoveho "instant ready" kroku -
        // a normalni boot pri tom zustane presne stejny jako pred B258.
        // Bezpecnejsi oprava = odstranit riskantni cast, nechat jen tu,
        // co je overene bezpecna.
        //
        // BUILD2SB83->84 (POUCNY OMYL, pak SKUTECNA OPRAVA):
        // Rene - "chybi zaverecne chrceni CSAVE, jen uvodni piskot je
        // tam." Domnival jsem se nejdriv, ze appka do sériove-vystupni
        // IRQ obsluhy ($EA88/$EAAD) nikdy nevstoupi, protoze potrebuje
        // "prvni jiskru", kterou jsem ja nikdy negeneroval - stravil
        // jsem hodne casu hledanim spravneho signalu k jejimu spusteni
        // (zkusil IRQEN bit4, pak SKCTL bit3 - obé NESPOLEHLIVE, oboje
        // zamitnuto testem). DELSIM SLEDOVANIM (zadne odhady) jsem
        // zjistil: appka do $EA88 dojde SAMA, prirozene, uplne bez
        // jakekoliv "opravy" - jen to trva pres 20 vterin (Timer1
        // preruseni na jinem miste, $EBC6). SKUTECNA PRICINA
        // chybejiciho "chrceni" byla jinde: byte-k-bytu zpozdeni
        // (drive "serStav/serOdpocet", ~2 scanline) bylo AZ 140x
        // rychlejsi nez ma byt - primo overeno v JS referenci
        // (jedinem zdroji pravdy pro logiku): "outBusy=10,
        // shiftBusy=30" scanline, NE 2. Cely 256+ bajtovy blok se tak
        // odesilal za 0.3s misto realistickych ~4.7s pri 600 baudech -
        // zvuk pak vypadal jako jeden nerozeznatelny impuls hned pri
        // konci, ne jako "chrceni" rozlozene v case. OPRAVA (nize u
        // SEROUT zapisu a v runScanline): outBusy/shiftBusy nahrazuji
        // puvodni serStav/serOdpocet, presne casovani z reference.
        irqen = v;
        irqst |= (~v) & 0xFF;                   // zakazane se rovnou zahodi
        obnovIrq();
        return;
      }
      if (r == 0x0F) { skctl = v; return; }
      return;
    }
    if (page == 0xD300) { mem.piaWrite(a, v); return; }
    if (page == 0xD400) {                       // ANTIC
      {
        // CHBASE, HSCROL a VSCROL se meni PRES DLI uprostred obrazu.
        // Kdyz se ctou az na konci snimku, cte se znakova sada z mista,
        // kde uz zadna neni - u Decathlonu vyslo CHBASE=$00 a misto
        // tabule byla kase ze systemovych promennych.
        const int r = 0x20 + (a & 0x0F);
        if (segN < SEG_MAX) {
          segCyk[segN] = (int)(cpu.c.cycles - lineCyc0);
          segReg[segN] = r; segHod[segN] = v & 0xFF; segN++;
        }
      }
      switch (a & 0x0F) {
        case 0x00: dmactl = v; return;
        case 0x01: chactl = v; return;
        case 0x02: dlistL = v; return;
        case 0x03: dlistH = v; return;
        case 0x04: hscrol = v; return;
        case 0x05: vscrol = v; return;
        case 0x07: pmbase = v; return;
        case 0x09: chbase = v; return;
        case 0x0A: {                            // WSYNC
          // Zapis do WSYNC ZASTAVI procesor az do konce radky. Bez toho
          // se rozsype casovani vsech kernelu, ktere synchronizuji na
          // paprsek - a to je skoro kazda hra.
          const long long cyk = cpu.c.cycles - lineCyc0;
          if (cyk < 105) cpu.c.cycles = lineCyc0 + 105;
          return;
        }
        case 0x0E: nmien = v; return;
        case 0x0F: nmist = 0x1F; return;        // NMIRES
      }
      return;
    }
  }

  void obnovIrq() {
    cpu.c.irqLine = (((~irqst) & irqen) & 0xFF) ? 1 : 0;
  }

  // BUILD2SB52: STIMER (zapis do $D209) na skutecnem POKEY vynuluje
  // vystupni citadla vsech kanalu - bez tohohle by po STIMER hrál
  // kazdy ton s nahodnou fazi misto od zacatku.
  void cnt0Reset() {
    pokeyAudio.cnt[0] = pokeyAudio.cnt[1] = pokeyAudio.cnt[2] = pokeyAudio.cnt[3] = 0;
  }

  // BUILD2SB52: vygeneruje 'n' vzorku zvuku (mono float, -1..1) z
  // AKTUALNIHO stavu registru - viz nap_atari_pokey.h pro cely
  // algoritmus (preklad z JS reference).
  //
  // BUILD2SB56: pred samotnym generovanim zkontrolovat, jestli se od
  // POSLEDNIHO volani zmenil bit reproduktoru (CONSOL bit3, $D01F) -
  // pokud ano, spustit kratky "klik" presne jako skutecny hardware
  // (viz PokeyAudioState::spkLevel/spkDecay a jejich pouziti v
  // nap_atari_pokey.h).
  // BUILD2SB77: puvodne tady byl "audioCyklusPocatek" (akumulator) -
  // ODSTRANEN, byl to presne ten drift popsany v komentari u
  // genAudio() nize. Pozice se ted pocita VZDY cerstve z aktualniho
  // cpu.c.cycles, zadny mezistav k udrzovani.

  // BUILD2SB75: Rene - "pri nabootovani mas kratky zvuk po resetu
  // dlouhy zvuk, presne obracene... nesmyslne to lupne i po prejiti
  // do self testu." Nalezena SPOLECNA pricina obou hlaseni: kdykoli
  // appka posune MNOHO snimku najednou synchronne (boot=600, self-
  // test=400) BEZ prubezneho volani genAudio() (to zacne az POTOM,
  // v hlavni smycce), VSECHNY zaznamenane prechody z tehle doby maji
  // cyklus VZDALENY od "audioCyklusPocatek" (ktery zustava na stare
  // hodnote, dokud genAudio() konecne neprobehne) - genAudio() je pak
  // VSECHNY namacka na konec prvniho bufferu (bezpecnostni orezani
  // pozice na max. n), misto aby byly rozlozene v case jak doopravdy
  // zazněly. Vysledek: bud "zmacknuty" kratky zvuk (misto rozlozeneho
  // dlouheho), nebo naopak nesmyslne velky pocet "kliku" najednou
  // (self-test). Oprava: po KAZDEM takovem velkem bloku snimku
  // (bootNative, self-test vstup, napsani textu+RETURN, reset)
  // srovnat sledovani zvuku s aktualnim stavem procesoru - zahodi to
  // presne prehrani zvuku BEHEM synchronniho bloku (nejde jinak bez
  // vetsi prestavby cele smycky), ale zabrani to spatnym, zmacknutym
  // nebo umele vysokym artefaktum v zaznamu HNED PO bloku.
  void srovnatSledovaniZvuku() {
    pocetSpeakerPrechodu = 0;
    gtiaSpeakerVidenaAudioGen = gtiaSpeakerBit;
  }

  // BUILD2SB77: Rene testoval na REALNEM zarizeni - "nejde zvuk pri
  // nabootovani, nejde zvuk pri csave/cload pipnuti." V CLI testu
  // (presne 882 vzorku/snimek, presne 1 genAudio() volani na 1
  // runFrame()) vsechno fungovalo. Na REALNEM telefonu to ale NEBEZI
  // v takhle dokonalem rytmu - JS smycka vola audioChunkNative() s
  // velikostmi podle SKUTECNEHO ubehleho casu (Web Audio API), ne
  // podle pevneho poctu snimku. SKUTECNA PRICINA: audioCyklusPocatek
  // (B268) byl AKUMULATOR - kazde volani genAudio() k nemu jen
  // PRICITALO "n*cyklu_na_vzorek", v predpokladu, ze presne tolik
  // CPU cyklu SKUTECNE ubehlo od minuleho volani. Na realnem
  // zarizeni tenhle predpoklad NEPLATI presne - drobne odchylky se
  // KAZDYM volanim SCITAJI (drift), az zaznamenane prechody
  // reproduktoru vypocitane vuci tomuhle "ujizdenemu" pocatku vyjdou
  // MIMO aktualni buffer a orezavaci pojistka (pozice<0 -> 0,
  // pozice>n -> n) je vsechny namacka na kraj - misto skutecneho
  // tónu jen umlcnuty/zkresleny vysledek. OPRAVA: NEAKUMULOVAT nic -
  // pocatek bufferu pocitat VZDY ZNOVU primo z aktualniho
  // cpu.c.cycles (jedine VZDY spravne, autoritativni cislo), ne z
  // odhadu postaveneho na predchozich volanich. Zadny drift nemuze
  // vzniknout, protoze se nikdy nic nesklada z minulosti.
  void genAudio(float *out, int n, double sampleRateHz) {
    // 1773447 Hz - presne stejna konstanta jako v JS referenci
    // ("CPS=1773447/ac.sampleRate", komentar tam "cyklu na vzorek (PAL)").
    const double cyklu_na_vzorek = 1773447.0 / sampleRateHz;
    // BUILD2SB77: pocatek TOHOTO KONKRETNIHO bufferu = aktualni cyklus
    // MINUS kolik cyklu zabira n vzorku - vzdy cerstve spocitano,
    // zadny akumulovany stav z minula.
    const long long pocatekBufferu = cpu.c.cycles - (long long)((double)n * cyklu_na_vzorek);

    // BUILD2SB74: zpracovat VSECHNY zaznamenane prechody na jejich
    // SPRAVNYCH pozicich uvnitr bufferu - misto jedine kontroly "je bit
    // ted jiny nez naposledy?" na zacatku. Bez tohohle se rychle
    // opakovane prepinani (skutecny tón z ROM pipaci smycky, ne jen
    // jednotlivy klik) ztratilo - zustal jen posledni stav na konci
    // snimku. Viz komentar u pole speakerPrechody.
    int zapsanoVzorku = 0;
    for (int i = 0; i < pocetSpeakerPrechodu; i++) {
      long long delta = speakerPrechody[i].cyklus - pocatekBufferu;
      int pozice = (int)(delta / cyklu_na_vzorek);
      if (pozice < zapsanoVzorku) pozice = zapsanoVzorku;   // poradi zachovano zapisem, jen pojistka
      if (pozice > n) pozice = n;
      if (pozice > zapsanoVzorku) {
        zapisUsekSDvoutonem(out, zapsanoVzorku, pozice - zapsanoVzorku, cyklu_na_vzorek, pocatekBufferu);
        zapsanoVzorku = pozice;
      }
      // prave TADY, na spravne pozici, spustit klik/tón - presne jako
      // predtim delalo genAudio() jen jednou za cely buffer.
      pokeyAudio.spkLevel = speakerPrechody[i].novaHodnota ? 1 : -1;
      pokeyAudio.spkDecay = (long)(sampleRateHz * 0.004); // ~4ms, presne jako JS reference
      gtiaKlikPocitadlo++;
    }
    gtiaSpeakerVidenaAudioGen = gtiaSpeakerBit;
    pocetSpeakerPrechodu = 0;
    if (zapsanoVzorku < n) {
      zapisUsekSDvoutonem(out, zapsanoVzorku, n - zapsanoVzorku, cyklu_na_vzorek, pocatekBufferu);
    }
  }

  // BUILD2SB88: Rene poslal SKUTECNOU nahravku (wav) realneho Atari
  // dvoutonoveho vysilani - PRIMO ZMERENO (autokorelace, ne odhad):
  // behem "piskotu" appka strida MEZI 594.0 Hz A 5278.0 Hz (ne mala
  // odchylka - obrovsky skok!). 594.0 Hz PRESNE odpovida spojenemu
  // kanalu 3+4 (audf3=204,audf4=5: (204+5*256+7)*1 cyklu -> 594.7 Hz
  // vypocteno). 5278.0 Hz PRESNE odpovida SAMOSTATNEMU kanalu 1
  // (audf1=5, standardni delic 28: (5+1)*28 cyklu -> 5278.1 Hz
  // vypocteno) - SHODA NA DESETINY Hz! Behem "datoveho toku" (skutecna
  // data) autokorelace ukazuje RYCHLE stridani 565/4009 Hz - stejny
  // MECHANISMUS (dva nezavisle kanaly), jen prepinane BIT PO BITU
  // misto zridkavych "urovnovych" bloku jako v piskotu.
  // ZAVĚR: skutecny POKEY dvoutonovy obvod NEMENI periodu jednoho
  // kanalu (predchozi B276 "pulena perioda" pristup byl SPATNY,
  // odstranen) - PREPINA, KTERY ZE DVOU NEZAVISLE BEZICICH OSCILATORU
  // (kanal 1 samostatne, VERSUS kanal 3+4 spojene) je prave "pripojen
  // na vystup" - presne jako multiplexer/gate. Oba oscilatory pritom
  // beží CELOU DOBU na svych vlastnich, NEZMENENYCH frekvencich -
  // meni se jen HLASITOST/GATING, ne frekvence.
  void zapisUsekSDvoutonem(float *out, int odkud, int pocet, double cyklu_na_vzorek, long long pocatekBufferu) {
    if (pocet <= 0) return;
    const bool dvouton = (skctl & 0x08) != 0;
    if (!dvouton) { nap::pokeyGenSamples(audf, audc, audctl, pokeyAudio, out + odkud, pocet, cyklu_na_vzorek); return; }
    const double cyklu_na_bit = 1773447.0 / 600.0; // 600 baudu, presne jako JS reference pro cteni pasky
    int zapsano = 0;
    while (zapsano < pocet) {
      long long cykl = pocatekBufferu + (long long)((double)(odkud + zapsano) * cyklu_na_vzorek);
      long long odBitu = cykl - serRamecStart;
      int bitIndex = odBitu >= 0 ? (int)(odBitu / cyklu_na_bit) : -1;
      int bit = 1; // mimo platny ramec (nebo po jeho konci) = klidova "1" uroven linky (kanal 1)
      if (bitIndex >= 0 && bitIndex < 10) bit = (serRamec >> bitIndex) & 1;
      long long konecTohotoBituCyklus = (bitIndex >= 0 && bitIndex < 10)
          ? serRamecStart + (long long)((double)(bitIndex + 1) * cyklu_na_bit)
          : (long long)1e18;
      int vzorkuDoHranice = (int)((double)(konecTohotoBituCyklus - cykl) / cyklu_na_vzorek);
      if (vzorkuDoHranice < 1) vzorkuDoHranice = 1;
      int kolikTeď = pocet - zapsano;
      if (kolikTeď > vzorkuDoHranice) kolikTeď = vzorkuDoHranice;

      // AUDF zustava NEZMENENE (oba oscilatory beží porad) - meni se
      // jen AUDC (hlasitost/gating): bit=1 -> slyset kanal 1 SAMOTNY
      // (5278Hz), kanal 2/3/4 ztlumeny; bit=0 -> slyset kanal 3+4
      // SPOJENY (594Hz), kanal 1/2 ztlumeny.
      int audcDvouton[4];
      if (bit) { audcDvouton[0]=audc[0]; audcDvouton[1]=0; audcDvouton[2]=0; audcDvouton[3]=0; }
      else     { audcDvouton[0]=0; audcDvouton[1]=0; audcDvouton[2]=audc[2]; audcDvouton[3]=audc[3]; }
      nap::pokeyGenSamples(audf, audcDvouton, audctl, pokeyAudio, out + odkud + zapsano, kolikTeď, cyklu_na_vzorek);
      zapsano += kolikTeď;
    }
  }

  // BUILD2SB65: POKEY casovace 1/2/4 - presny preklad z JS reference
  // (timerPeriod/timersReload/timersTick). Perioda v CPU cyklech
  // (1,79 MHz), stejny vzorec jako pouziva samotna syntéza zvuku
  // (viz nap_atari_pokey.h) - je to STEJNY hardware, jen jiny ucel
  // (tady odpocet do preruseni, tam generovani tonu).
  long timerPeriod(int ch) const {
    const int ac = audctl;
    if (ch == 0) {
      if (ac & 0x10) return 0;                              // 1+2 spojene: ridi kanal 2
      const int d = (ac & 0x40) ? 1 : ((ac & 1) ? 114 : 28);
      return (long)(audf[0] + ((ac & 0x40) ? 4 : 1)) * d;
    }
    if (ch == 1) {
      if (ac & 0x10) {
        const int d = (ac & 0x40) ? 1 : ((ac & 1) ? 114 : 28);
        return (long)((audf[1] << 8) + audf[0] + ((ac & 0x40) ? 7 : 1)) * d;
      }
      const int d = (ac & 1) ? 114 : 28;
      return (long)(audf[1] + 1) * d;
    }
    if (ch == 3) {
      if (ac & 8) {
        const int d = (ac & 0x20) ? 1 : ((ac & 1) ? 114 : 28);
        return (long)((audf[3] << 8) + audf[2] + ((ac & 0x20) ? 7 : 1)) * d;
      }
      const int d = (ac & 1) ? 114 : 28;
      return (long)(audf[3] + 1) * d;
    }
    return 0;
  }
  /** Zapis do STIMER ($D209) znovunabiji vsechny tri casovace. */
  void timersReload() {
    timer1Cyc = timerPeriod(0);
    timer2Cyc = timerPeriod(1);
    timer4Cyc = timerPeriod(3);
  }
  /** Vola se jednou za radku (114 cyklu) - presne jak to dela reference. */
  void timersTick(int cyk) {
    if (!(irqen & 0x07)) return; // nikdo neposlouchá, netreba pocitat
    if (timer1Cyc > 0) {
      timer1Cyc -= cyk;
      if (timer1Cyc <= 0) { long p = timerPeriod(0); timer1Cyc += (p > 0 ? p : 1000000000L); irqst &= ~0x01; obnovIrq(); }
    }
    if (timer2Cyc > 0) {
      timer2Cyc -= cyk;
      if (timer2Cyc <= 0) { long p = timerPeriod(1); timer2Cyc += (p > 0 ? p : 1000000000L); irqst &= ~0x02; obnovIrq(); }
    }
    if (timer4Cyc > 0) {
      timer4Cyc -= cyk;
      if (timer4Cyc <= 0) { long p = timerPeriod(3); timer4Cyc += (p > 0 ? p : 1000000000L); irqst &= ~0x04; obnovIrq(); }
    }
  }

  /** Stisk klavesy: OS ji prevezme pres preruseni z POKEY. */
  void klavesa(int kod) {
    kbcode = kod & 0xFF;
    irqst &= ~0x40;
    obnovIrq();
  }

  int dlistAddr() const { return (dlistL | (dlistH << 8)) & 0xFFFF; }

  void reset() {
    // BUILD2SB82: Rene - "RESET na realnem Atari zachovava napsany
    // kod, jen vymaze obrazovku - POWER maze vse a bootuje znovu."
    // NALEZENA A PRESNE OVERENA PRICINA: "mem.pia = Pia();" tady
    // RESETOVALO CELOU PIA VCETNE PORTB (rizeni bankovani pameti na
    // 130XE)! Na REALNEM hardwaru RESET signal PIA vubec neresetuje -
    // PORTB je jen softwarova zapadka, prezije reset stejne jako
    // zbytek RAM. Kdyz se PORTB pri resetu zmenilo, appka se najednou
    // divala na JINOU BANKU pameti nez tu, kde byl napsany program -
    // ten pak vypadal "ztraceny", i kdyz fyzicky nikde nezmizel.
    // PRIMO OVERENO testem: napsat "10 PRINT HI", zavolat reset BEZ
    // tehle radky, LIST po resetu SPRAVNE ukazal puvodni program
    // (predtim, s "mem.pia = Pia();", LIST po resetu ukazal PRAZDNO).
    // JS reference potvrzuje - "M.reset=function(){ cpu.reset(); };"
    // - opravdu jen CPU, nic jineho.
    cpu.c = CpuState();
    cpu.reset();
    // BUILD2SB77: cpu.c.cycles se vynuluje uvnitr CpuState() vyse -
    // stare zaznamenane prechody (z PRED resetem, s VELKYMI cyklovymi
    // hodnotami) by vuci novemu, vynulovanemu cpu.c.cycles vysly jako
    // "daleko v budoucnosti" - zahodit je, at genAudio() nezacne
    // spatne.
    pocetSpeakerPrechodu = 0;
  }

  void dalsiDlInstrukce() {
    for (int ochrana = 0; ochrana < 8; ochrana++) {
      if (++dlKroku > 300) { dlMode = 0; dlZbyva = 240; dlDli = false; return; }
      const int op = mem.cpuRead(dlPc); dlPc = (dlPc + 1) & 0xFFFF;
      const int m = op & 0x0F;
      dlDli = (op & 0x80) != 0;
      if (m == 0) { dlMode = 0; dlZbyva = ((op >> 4) & 7) + 1; dlRadek = 0;
                    dlHskrol = false; dlVskrol = false; return; }
      // POZOR NA PORADI: bit4 je SVISLE, bit5 je VODOROVNE skrolovani.
      // Mel jsem to prohozene, takze se u rezimu s vodorovnym skrolem
      // menil pocatecni radek znaku misto sirsiho nataceni z pameti.
      dlVskrol = (op & 0x10) != 0;      // bit4 = svisle
      dlHskrol = (op & 0x20) != 0;      // bit5 = vodorovne

      if (m == 1) {
        // Skok. Adresa je u OBOU variant - to jsem drive u $01 vubec necetl.
        const int lo = mem.cpuRead(dlPc); dlPc = (dlPc + 1) & 0xFFFF;
        const int hi = mem.cpuRead(dlPc); dlPc = (dlPc + 1) & 0xFFFF;
        dlPc = lo | (hi << 8);
        if (op & 0x40) {                            // $41 = JVB: KONEC SNIMKU
          dlKonec = true; dlMode = 0; dlZbyva = 240; dlRadek = 0;
          return;                                   // dal uz se nekresli
        }
        continue;                                   // $01 = JMP: cti dal
      }
      if (op & 0x40) {                              // LMS
        const int lo = mem.cpuRead(dlPc); dlPc = (dlPc + 1) & 0xFFFF;
        const int hi = mem.cpuRead(dlPc); dlPc = (dlPc + 1) & 0xFFFF;
        dlScreen = lo | (hi << 8);
      }
      dlMode = m; dlRadek = dlVskrol ? (vscrol & 15) : 0;
      dlZbyva = AnticView::popisRezimu(m).scanline - dlRadek;
      if (dlZbyva <= 0) dlZbyva = 1;
      return;
    }
    dlMode = 0; dlZbyva = 240; dlDli = false;
  }

  /** Hodnota registru r v okamziku, kdy paprsek prochazel bodem x. */
  int regNaX(int r, int x) const {
    // 1 cyklus procesoru = 2 barvove takty = 4 body ve framebufferu,
    // a bod x=0 odpovida barvovemu taktu 32
    int v = segZac[r];
    for (int i = 0; i < segN; i++) {
      if (segReg[i] != r) continue;
      const int xz = segCyk[i] * 4 - 64;
      if (x >= xz) v = segHod[i];
    }
    return v;
  }

  // Jedna scanline. PAL: 312 radku po 114 cyklech.
  void runScanline() {
    lineCyc0 = cpu.c.cycles;
    segN = 0;
    for (int p = 0; p < 4; p++) {
      segZac[p] = hposp[p]; segZac[4+p] = hposm[p];
      segZac[8+p] = sizep[p]; segZac[0x0D+p] = grafp[p];
      segZac[0x12+p] = colpm[p]; segZac[0x16+p] = colpf[p];
    }
    segZac[0x0C] = sizem; segZac[0x11] = grafm;
    segZac[0x1A] = colbk; segZac[0x1B] = prior;
    segZac[0x20] = dmactl; segZac[0x21] = chactl;
    segZac[0x24] = hscrol; segZac[0x25] = vscrol;
    segZac[0x27] = pmbase; segZac[0x29] = chbase;

    bool dliTed = false;
    if (line == 8) {
      dlPc = dlistAddr(); dlScreen = -1; dlZbyva = 0; dlKroku = 0; dlRadek = 0; dlKonec = false;
      if (view) view->vymaz(colbk);
    }
    // ANTIC si nacte instrukci display listu na zacatku radky
    int mode = 0, screen = -1, radek = 0;
    const bool viditelna = (line >= 8 && line < 248 && view);
    if (viditelna && (dmactl & 3) && !dlKonec) {
      if (dlZbyva == 0) dalsiDlInstrukce();
      mode = dlMode; screen = dlScreen; radek = dlRadek;
    }

    // TEPRVE TED procesor - behem nej se zaznamenavaji zapisy do GTIA
    const long long konec = lineCyc0 + 114;
    while (cpu.c.cycles < konec && !cpu.c.jam) cpu.step();

    if (viditelna) {
      const int y = line - 8;
      // ANTIC vyrobi proud kodu, GTIA z nej udela obraz
      if (stopa && y < 240) { stopaMode[y] = mode; stopaScr[y] = screen; stopaRad[y] = radek; }
      if ((dmactl & 3) && !dlKonec && mode != 0)
        view->anRadek(mem, mode, screen, radek, segZac[0x20], segZac[0x29],
                      dlHskrol, segZac[0x24]);
      else
        view->anPozadi();
      view->gtiaRadek(mem, y, line, *this);
      if ((dmactl & 3) && !dlKonec) {
        dlRadek++;
        if (--dlZbyva == 0) {
          if (dlMode != 0 && dlScreen >= 0) dlScreen += AnticView::bajtuNaRadek(dlMode, dmactl, dlHskrol);
          dliTed = dlDli;
        }
      }
    }
    if (dliTed && (nmien & 0x80)) {
      nmist = (nmist & 0x3F) | 0x80;
      cpu.c.nmiPending = true;
    }
    // POKEY: sériový vystup - presne z JS reference, dva nezavisle
    // citace, kazdy snizeny o 1 kazdou scanline.
    bool zmenaSerIrq = false;
    if (outBusy > 0 && --outBusy == 0) { irqst &= ~0x10; zmenaSerIrq = true; }
    if (shiftBusy > 0 && --shiftBusy == 0) { irqst &= ~0x08; zmenaSerIrq = true; }
    if (zmenaSerIrq) obnovIrq();
    // BUILD2SB65: POKEY casovace 1/2/4 - jednou za radku (114 cyklu),
    // presne jak to dela JS reference ("vola se kazdou scanline").
    timersTick(114);
    line++;
    if (line == 248) {
      nmist = (nmist & 0x3F) | 0x40;
      if (nmien & 0x40) cpu.c.nmiPending = true;
    }
    if (line >= 312) { line = 0; frame++; }
  }

  void runFrame() { const long long f = frame; while (frame == f) runScanline(); }
};

} // namespace nap
