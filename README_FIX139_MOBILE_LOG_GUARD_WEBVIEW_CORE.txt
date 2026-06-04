AtariHelp.eu EMU-09
FIX139_MOBILE_LOG_GUARD_WEBVIEW_CORE

Cíl:
- Stabilizační build pro Android WebView / telefon.
- Žádný nový vizuální hack a žádná změna obrazu proti FIX137/FIX138 baseline.
- Oprava výkonového rizika: starý log zapisoval opakované loader handoff hlášky do DOM/localStorage příliš často.
- Ochrana proti možnému WebView/GPU problému po zavření aplikace: při schování/zavření stránky se zastaví RAF smyčka, ztiší audio a ořízne obří DOM log.

Co se změnilo:
1) Log/DOM guard
- Opakované FIX125 LOADER HANDOFF hlášky se už necpou stovkykrát do viditelného logu.
- localStorage se nepřepisuje při každém diagnostickém řádku, ale jen rozumně a při snapshotu/chybě.
- Snapshot obsahuje nový řádek: LOG GUARD FIX139.

2) WebView cleanup
- visibilitychange / pagehide / beforeunload zastaví emulaci a uvolní audio/canvas/log tlak.
- Cíl: zabránit tomu, aby WebView po návratu do Seznamu/Chrome nechal rozbitý textový/GPU kompozitor.

3) Mobilní výkon
- Donkey/Cobra render zůstává každou frame, aby nebyl pocit trhanosti z renderEvery skipu.
- Těžké fallback profily dál mohou renderovat méně často.
- Audio update na mobilu je zlehčený.

Build tag:
FIX139_MOBILE_LOG_GUARD_WEBVIEW_CORE

Commit summary:
FIX139 mobile log guard webview core

Test plan:
1. Na mobilu A12: krátce Donkey Kong a Super Cobra, hlavně jestli je pohyb méně kousavý.
2. Ukonči aplikaci / přepni do Seznamu nebo Chrome: ověř, jestli se už nerozsype písmo.
3. Moon Patrol jen jeden snapshot kvůli LOG GUARD FIX139 a MOBILE PERF FIX139.
4. Donkey Kong: menu + dupající intro + hra.
5. Super Cobra: hratelnost, scrolling krajiny a kulomet jen orientačně.

Poznámka:
Pokud se písmo v Seznamu/Chrome rozsype i po FIX139, je to téměř jistě WebView/Chrome/GPU kompozitor problém. Aplikace by cizí aplikace přímo měnit neměla, ale sdílený WebView/Chromium renderer a GPU layer může po přetížení zanechat artefakty do refresh/restartu.
