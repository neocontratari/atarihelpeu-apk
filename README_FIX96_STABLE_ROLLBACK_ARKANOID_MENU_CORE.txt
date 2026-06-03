AtariHelp.eu EMU-09 FIX96 STABLE ROLLBACK + ARKANOID MENU CORE

FIX96 je bezpečný stabilizační balík po regresi ve FIX95. Vrací DLIST výběr a frame-hold chování na stabilnější FIX94 cestu pro Donkey/Cobra a ponechává jedinou zjevnou výhru z FIX95: Arkanoidovi se automaticky nemačká START/FIRE, takže menu má šanci zůstat viditelné.

Co je záměrně vypnuté proti FIX95:
- agresivní Cobra/Donkey scan-DLIST promotion, která rozbila Donkey intro/menu a Cobra obraz,
- agresivní odmítání/pamatování DLISTů podle FIX95 stability guardu.

Co zůstává:
- FIX94 input/POT shadow core,
- snapshot diagnostika opravená přes VIEWPORT DIAG FIX96,
- Arkanoid no-auto-skip menu guard,
- obecný směr: žádné ruční úpravy, ZIP overlay pro GitHub Desktop.

Commit Summary:
FIX96 stable rollback Arkanoid menu core

Test priority:
1) Donkey Kong menu/intro/gameplay: potvrdit návrat minimálně na FIX94 stav.
2) Super Cobra menu + scroll: potvrdit návrat minimálně na FIX94 stav.
3) Arkanoid III: 3–5 sekund nic nemačkat, screenshot menu, potom TAP FIRE/START a snapshot.
4) Montezuma: screenshot hráč/lebka + snapshot pro další PMG/pozicování.
