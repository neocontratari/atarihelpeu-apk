AtariHelp.eu EMU-09 FIX200_CAS_WAV_DLI_TIMING_CORE

Commit summary:
FIX200 cas wav dli timing core

Co je ve FIX200:
- navazuje na FIX199 HW/SDLST DLIST trust, ale nevraci profilove DLIST sablony
- pridana samostatna diagnosticka vetev CAS/WAV PILOT DONKEY / CAS/WAV PILOT COBRA
- CAS/WAV pilot zatim neni fyzicky FSK audio dekoder; je to oddelena cassette-style staging vetev pro porovnani loader/init timingu proti cistemu XEX
- v hardware purge rezimu se vypina synteticky soft DLI pro profil Donkey/Cobra/Montezuma, pokud DLIST nema skutecne $80 DLI bity
- Super Cobra DLI budget je zvysen, aby se DLI retezec nedosekl v polovine radku
- RAM DLIST scan zustava jen diagnosticky, nesmi prebit HW/SDLST nebo segment DLIST
- BASIC direct pro hotove hry zustava OFF
- G7/GTIA/BCD/ZVUK/BASIC paste zustavaji z overeneho zakladu

Test plan:
1. TEST SUPER COBRA XEX - porovnej menu a hru proti FIX199, sleduj sum.
2. TEST DONKEY KONG XEX - porovnej proti FIX199, jestli se obraz zlepsil/zhorsil.
3. CAS/WAV PILOT COBRA - jestli je obraz stejny jako XEX, problem je renderer/DLI; pokud jiny, problem je loader/init timing.
4. CAS/WAV PILOT DONKEY - stejne porovnani.
5. Snapshot/log: hledej XEX STREAM STATUS FIX200, CAS/WAV PILOT FIX200, DLI CAP FIX200, DLIST RESOLVE FIX200.
