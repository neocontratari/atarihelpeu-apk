AtariHelp.eu APK Builder — EMU-09 FIX51 BLANK DLI + PMG CORE

Směr projektu: obecný Atari XEX emulátor pro vlastní hry z telefonu, ne jen profilové hacky pro vestavěné testy.

FIX51 opravuje zásadní chybu z FIX50: DLI bit se musí obsloužit i na blank instrukcích display listu ($F0 apod.). Pitstop II používá horní blank DLI k nastavení CHBASE/barev před viditelným obrazem, takže FIX50 kreslil horní část špatným stavem. Log nově vypisuje DLI SCANLINES a PMG INFO pro ladění River/Donkey/vlastních XEX.
