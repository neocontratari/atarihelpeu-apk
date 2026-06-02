# AtariHelp.eu EMU-09 FIX78 GALAXIAN INIT + DLI CORE

FIX78 navazuje na FIX77 a resi hlavne `Galaxian (Title Version 2).xex`.

Hlavni opravy:
- INITAD-only XEX bez RUNAD uz nespadne hned na prazdnem DOS/loader BRK v nizke pameti, pokud uz ma platny DLIST a NMI/DLI.
- DLI/VBI async wrapper konci hned po RTI/RTS a obnovuje stack page, aby DLI rutina neposkodila hlavni CPU stack.
- Log pridal INITIDLE/INIT-ONLY IDLE diagnostiku.

Summary pro GitHub Desktop:
`FIX78 galaxian init dli core`
