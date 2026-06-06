AtariHelp.eu EMU-09 FIX255 SNAPSHOT CLOAD STATE LOCK CORE

Commit summary:
FIX255 snapshot CLOAD state lock core

CIL:
- Opravit snapshot: text se MUSI vzdy ukazat primo v aplikaci v textarea.
- Pridat KOPIROVAT SNAPSHOT a OZNACIT SNAPSHOT.
- TXT download se stale zkusi, ale uz neni jedina cesta.
- CLOAD + PLAY vzdy cisti starou STOP/EOF stopku a startuje WAV od 0:00.
- STOP/RESET/PAGEHIDE zabiji vsechny audio elementy.
- Stale NO FAKE LOAD: TurboBasic se neinjektuje do RAM.
- SELF TEST path je zachovana, bez fake kresleni.

KODY JSOU STEJNE
