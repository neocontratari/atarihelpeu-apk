EMU-09 FIX49 PITSTOP PHASE SNAPSHOT

Základ: FIX48/FIX45/FIX44/FIX39.

Oprava: Pitstop II už nepoužívá jeden starý video snapshot přes všechny fáze hry. Snapshot je svázaný s fází CPU:
- title_wait
- name_wait
- select_wait ($30C9/$30CC)
- race_wait ($4F78/$4F7B)

Tím se má zabránit tomu, aby snímek trati překryl menu výběru trati/hráčů.
