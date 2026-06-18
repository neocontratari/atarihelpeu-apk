ATARIHELP.EU EMU-10 BUILD2EI_REAL_AUDIO_CORE_CLEAN

KODY JSOU STEJNE.

Tento balik neni Decathlon/grafika. Je to kazetova oprava bez lakovani:
- meni se jen app/src/main/assets/emu/index.html
- Java, ROM, XEX, UI a hlavni app menu se nemení
- live WebAudio monitor pri CSAVE je vypnuty
- LAST CSAVE shortcut je vypnuty, aby se CLOAD nelakoval pres pamet emulatoru
- CAS se uklada po skutecnych 132B Atari DATA chunkech
- WAV export vraci DL fyzicke parametry: 48 kHz, 600 baud, MARK 5327 Hz, SPACE 3995 Hz, 12s leader, 0.30s IRG, 2s tail, amp 50%

PRESNY POSTUP TOTAL COMMANDER:
1) Vlevo otevri:
   C:\Users\neocontr\Documents\GitHub\atarihelpeu-apk\
2) Vpravo otevri ZIP a slozku build2ei_cmd_installer.
3) Zkopiruj vlevo do korene projektu tyto 3 veci:
   SPUSTIT_BUILD2EI_REAL_AUDIO_CORE_CLEAN.cmd
   _EMU10_BUILD2EI_PAYLOAD
   README_BUILD2EI_PRESNY_POSTUP.txt
4) Vlevo spust:
   SPUSTIT_BUILD2EI_REAL_AUDIO_CORE_CLEAN.cmd
5) GitHub Desktop:
   Summary: BUILD2EI real audio core clean
   Commit to main
   Push origin
6) WWW/GitHub Actions vyrobi APK.

CO MUSI BYT V LOGU:
AtariHelp.eu EMU-10 BUILD2EI_REAL_AUDIO_CORE_CLEAN pripraven.
CSAVE CAPTURE BUILD2EI: ... LIVE MONITOR JE VYPNUTY
SAVE BUILD2EI: zadny LAST-CSAVE shortcut do CLOAD
SAVE WAV BUILD2EI: REAL_AUDIO_CORE export ... amp=50%

TEST REAL ATARI:
Po CSAVE vezmi ulozeny WAV z Downloads/AtariHelp a pust ho na realnem Atari 130XE.
Pokud zvuk zase neodpovida a real Atari da ERROR 138, chyba je porad v audio/POKEY ceste - neposouvat jako hotove.
