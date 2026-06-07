AtariHelp.eu EMU-09 FIX267 ROM QUARANTINE SAFE CORE

Smer dopredu z posledni funkcni UI vetve FIX261.
Neni pouzit FIX262/FIX264/FIX266 rozbity CPU/audio/fast hook.

Ucel:
- vyrazit duplicitni / podezrele CO ROM assety z APK:
  app/src/main/assets/co24947a.rom
  app/src/main/assets/co60302a.rom
  app/src/main/assets/co61598b.rom
  app/src/main/assets/rom_audit/co24947a.rom
  app/src/main/assets/rom_audit/co60302a.rom
  app/src/main/assets/rom_audit/co61598b.rom
- ponechat stabilni BASIC/OS runtime pres hlavni ATARIXL.ROM + ATARIBAS.ROM, jinak by PURE ROM BASIC nemel z ceho bootovat.
- zadny fake READY, fake SELF TEST, fake LOAD, RAM inject ani herni hack.

Pozor:
ZIP overlay sam o sobe neumi smazat soubory, ktere uz jsou v lokalnim repo.
Proto je v baliku REMOVE_CO_ROMS_FROM_REPO.cmd. Spustit z korene repo po rozbaleni overlaye.
Skript maze jen sest presnych cest vyse a nic jineho.
