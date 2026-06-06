AtariHelp.eu EMU-09 FIX215 PURE REALROM BOOT CORE

KODY JSOU STEJNE.
SUBMARINE BASIC, GTIA 9/10/11 a ostatni BASIC testy nejsou menene.

FIX215 meni jen ROM boot cestu:
- do assets jsou znovu vlozeny presne ROM soubory od Reneho: ATARIXL.ROM, ATARIOSB.ROM, ATARIBAS.ROM.
- pridana tlacitka PURE ROM SELF TEST a PURE ROM BASIC BOOT.
- PURE rezim startuje CPU primo z RESET vectoru ATARIXL.ROM.
- v PURE rezimu jsou vypnute stare FIX181 guardy: BASIC AUTO jump, SIO timeout guard, RAMTEST guard, C3C1 cartridge skip, START kick.
- zadny fake READY ani fake SELF TEST obraz.

Ocekavany test:
1) PURE ROM SELF TEST.
2) Screenshot + snapshot.
3) Pokud nebezi SELF TEST, log ukaze skutecne PC a ROM cteni bez maskovani.
