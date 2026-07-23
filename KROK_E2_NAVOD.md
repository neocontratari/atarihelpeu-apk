# KROK E2 — opravy

## 1) Actions zase nejely — moje vina

Minule jsem do workflow přidal řádek, aby se běhy jmenovaly podle tvého
popisku. Od té chvíle to přestalo spouštět. **Vrátil jsem workflow přesně
do stavu, kdy fungoval.** Pořádek v názvech vyřešíme jinak — hlavní je,
aby ti to stavělo.

Navíc: přidal jsem, že se hotové APK maže po 3 dnech. Máš přes 1380 běhů
a každý po ~39 MB — je možné, že ti došlo místo na uložené soubory,
a GitHub pak nové běhy odmítá. Tohle tomu předejde.

**Kdyby to zase nešlo:** v Actions vlevo klikni na „Build APK" a nahoře
vpravo zkontroluj, jestli tam není nápis „Enable workflow" (= je vypnutý).

## 2) Nešlo vyskočit z PS1 — taky moje vina

Tvoje původní zobrazovací vrstva měla nastaveno, že nereaguje na dotyky,
aby propouštěla klepání na menu nad sebou. Já to na naší nové vrstvě
zapomněl nastavit, takže požírala dotyky a tlačítko na odchod nefungovalo.
Opraveno — teď je nastavená stejně jako ta tvoje původní.

## 3) Zůstávají opravy z minule

- hladké zvětšení (konec kostiček)
- poměr stran vždy 4:3 (opraveno to roztažené intro)
- zrušená přísná brzda (jela jen 15 FPS)
- tlačítko na logu NaP přepíná 4:3 ↔ 16:9

## Verze: EMU10-E2-DOTYKY-OPRAVENY

## Co mi pošli

- Naběhly Actions?
- Jde teď vyskočit z PS1 zpátky do aplikace?
- Je obraz hladší a sedí poměr stran?
- Kolik FPS ukazuje cast?
