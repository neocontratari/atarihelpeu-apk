FIX191_CLEAN_XEX_LOADER_PASTE_CORE

- Navazuje na potvrzeny FIX187/FIX188/FIX189 baseline.
- FIX190 REAL OS DIRECT XEX test selhal: Donkey spadl do BASIC/CART oblasti a Cobra do BRK/PC $0002.
- FIX191 proto vypina DIRECT ROM/BASIC pro hry a vraci Donkey/Cobra do cisteho protected XEX segment loaderu.
- ROMky zustavaji pro REAL OS/BASIC testy, ne pro hotove XEX hry.
- Pridany CISTY XEX START+OPTION test: jen vstupni puls START+OPTION po cold resetu, bez BASIC ROM direct.
- BASIC TXT / VLOZIT PROGRAM ma jeste delsi pauzy pro mobil/Nox: RETURN 1500 ms, znak 85 ms.
- Zvuk/POKEY bridge, G7 tunnel a GTIA 9/10/11 BARS zustavaji beze zmen.
- WAV/CAS loader je dalsi mozny smer, ale neni v tomhle buildu; nejdriv stabilizujeme cisty XEX loader.
