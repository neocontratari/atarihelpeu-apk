EMU-09 FIX42 Pitstop Stable Wait Core

Základ: FIX39/FIX36. Žádný PMBASE align z FIX37, žádný DLI chain z FIX38, žádné hromadné čištění latchů z FIX40.
Oprava: Pitstop II wait loop $4F78 uvolňuje jen aktuální $50AE,X podle CPU registru X.
PiTT-KiTT zůstává referenční hra.
