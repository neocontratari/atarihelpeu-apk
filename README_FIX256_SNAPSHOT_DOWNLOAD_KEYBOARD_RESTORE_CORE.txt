AtariHelp.eu EMU-09 FIX256 SNAPSHOT DOWNLOAD KEYBOARD RESTORE CORE

Commit summary:
FIX256 snapshot download keyboard restore core

CIL:
- Opravit snapshot tak, aby sel zase ULOZIT, nejen videt v textarea.
- Snapshot zkousi Android bridge, saveTextReportToMobile, Blob download a vytvori rucni odkaz STAHNOUT SNAPSHOT TXT.
- Textarea zustava hlavni zaloha.
- Vratit praktickou Atari klavesnici primo do test panelu.
- Pridat FULL RESET KAZETA/ZVUK, aby se smazal stop/eof latch a audio stav.
- CLOAD/WAV zustava poctivy runner bez RAM injectu.
- SELF TEST path preserved.

KODY JSOU STEJNE
