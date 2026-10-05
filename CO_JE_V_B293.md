# Co je v B293 – kontrola, že jádro je opravdu Atari 130XE

Tvá slova: „Ty děláš jádro na 800 – ale my musíme mít jádro na 130XE –
udělej kontrolu.“

## Odkud se vzalo „800“

- **atari800** je jen jméno emulátoru (tak se jmenuje od 90. let), emuluje
  všechny modely. Jako referenci jsem ho celou dobu spouštěl **jako 130XE PAL**
  (`-xe -pal`, XL/XE OS).
- **Acid800** je jen název testovací sady od autora Altirry – testuje i
  hardware XL/XE (např. test „MMU: XL banking“ – bankování paměti).

Tím ale kontrola nekončí. Prošel jsem bod po bodu všechno, čím se 130XE liší
od Atari 800, a **našel 4 věci, které se chovaly jako u 800 / 800XL**. Ty jsou
opravené.

## Co má 130XE jinak než 800 – a jak je to v jádře

| věc | Atari 800 | Atari 130XE | jádro |
|---|---|---|---|
| paměť | max. 48 kB | 64 kB + 64 kB rozšířené (4 banky po 16 kB v $4000–$7FFF) | ✅ |
| PORTB ($D301) | joysticky 3 a 4 | řízení paměti: bit 0 OS ROM, bit 1 BASIC, bity 2–3 banka, bit 4 procesor, bit 5 ANTIC, bit 7 self-test | ✅ |
| ANTIC a rozšířená paměť | – | ANTIC má vlastní přepínač (bit 5), může kreslit z jiné banky než procesor | ✅ ověřeno |
| ROM | OS-B 10 kB, BASIC na cartridge | OS XL/XE rev. 2 16 kB ($9211), BASIC rev. C vestavěný, self-test ROM | ✅ – ROM jsou z tvého 130XE |
| self-test ROM $5000–$57FF | – | PORTB bit 7 = 0; vidí ji procesor **i ANTIC** | 🔧 **opraveno** (ANTIC ji neviděl) |
| TRIG3 ($D013) | tlačítko joysticku 4 | **čidlo cartridge**: 0 = ve slotu nic není | 🔧 **opraveno** (bylo 1 = „cartridge zasunuta“ – např. M.U.L.E. pak nefunguje) |
| tlačítko RESET | jen NMI přes ANTIC, PIA zůstane | přímo resetuje procesor, ANTIC **i PIA/MMU** (PORTB → $FF, OS pak při teplém startu zapne BASIC) | 🔧 **opraveno** (PIA zůstávala jako u 800) |
| čtení neobsazené adresy ($D100, $D500–$D7FF) | „plovoucí“ sběrnice | „plovoucí“ sběrnice (800XL má $FF) | 🔧 **opraveno** (vracelo $FF jako 800XL) |
| paměť po zapnutí | – | vzor DRAM 130XE (bloky po 64 B: 80 FF 80 FF… / 00 7F 00 7F…) | 🔧 **doplněno** (byly nuly) |
| klávesnice | bez HELP | HELP (KBCODE $11), bez F1–F4 | ✅ |
| joystickové porty | 4 | 2 (PORTA, TRIG0/1) | ✅ |
| video | PAL/NTSC | PAL: 312 řádků, GTIA PAL ($D014 = 1) | ✅ |
| POKEY | 1,79 MHz NTSC | PAL 1,773 MHz | ✅ |

## Ověřeno (na počítači, skutečný OS a BASIC z 130XE)

Nový test `./test_core xe130` – **9 kontrol, 0 chyb**:

- program v BASICu zapíše do každé ze 4 bank jiné číslo a do základní paměti
  99, pak je přečte: **`10 11 12 13 99`** – banky i základní paměť jsou
  oddělené, přesně jako na 130XE,
- `PEEK(53267)` (TRIG3) = **0**,
- ANTIC a procesor vidí současně různé banky (PORTB bit 4 / bit 5),
- self-test ROM vidí procesor i ANTIC,
- RESET: program v paměti zůstal a OS znovu zapnul BASIC (`PEEK(54017)` = 253),
- paměť po zapnutí má vzor DRAM 130XE.

Samokontrola jádra (LOG/CHYBA): paměť má teď **stejné číslo jako staré jádro**
(`D3949DC5`) – mapování paměti 130XE přes všech 256 hodnot PORTB (procesor
i ANTIC) je bit po bitu stejné. Procesor `29C55806`.

V logu appky je po zapnutí HELP řádek `B293 JADRO = ATARI 130XE PAL: …`
s tím, co přesně se emuluje.

Všechny předchozí testy dál procházejí (Acid800 42/58, CSAVE → CLOAD, hry,
JNI testy 24 + 11 kontrol, 0 chyb).
