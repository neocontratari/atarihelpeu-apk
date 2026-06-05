FIX189_XEX_VISIBLE_INPUT_BRIDGE_CORE

FIX188_XEX_BRIDGE_AUDIO_INPUT_CORE

AtariHelp.eu EMU-09 FIX185_REALOS_GRAPHICS7_FULLSAFE_GTIA_BARS_CORE

Navazuje na FIX184.

Co je nove:
- klavesnice beze zmeny
- GTIA 9/10/11 BARS ponechane jako diagnosticke pruhy mimo BASIC/OS RAM
- GRAPHICS 7 rozdelene na realne dva pripady:
  - GRAPHICS 7 = split screen, grafika bezpecne Y 0..79 + textove okno
  - GRAPHICS 23 = GRAPHICS 7 + 16, full screen Y 0..95 bez textoveho okna
- tlacitko G7 SPLIT 80 TEST testuje realny split-screen rozsah
- tlacitko G7 FULL 96 TEST testuje plny 160x96 rozsah pres GRAPHICS 23
- BASIC TXT / VLOZIT PROGRAM ma G7 FULLSAFE ochranu: kdyz program ma GRAPHICS 7 a pouziva Y 95, prepise GRAPHICS 7 na GRAPHICS 23, aby se vyhnul ERROR-141
- zadne herni hacky

Commit summary:
FIX185 realos graphics7 fullsafe gtia bars core

Test plan:
1. READY / RESET rychle.
2. G7 SPLIT 80 TEST.
3. G7 FULL 96 TEST.
4. VLOZIT PROGRAM s rucnim G7 programem, ktery pouziva Y=95.
5. GTIA 9 BARS, GTIA 10 BARS, GTIA 11 BARS.
6. Snapshot/log: hledat FIX185 CORE, G7 FULLSAFE a GTIA BARS FIX185.

FIX187_REALOS_CPU_DECIMAL_G7_TUNNEL_CORE
- CPU decimal SBC/ADC fix for Atari BASIC FOR/NEXT loops.
- Designed to stop legal GRAPHICS 7 programs from overrunning loop limits and throwing ERROR-141.


FIX197: total purge legacy/profile DLIST render paths. XEX loader must log STREAMED=OK ACTIVE=YES.
