# KROK E — hladký obraz, správný poměr 4:3, přepínač 16:9

Parťáku, tvoje tři postřehy byly přesné a všechny měly příčinu u mě.

## 1) Kostičkovaná grafika — moje volba

Nastavil jsem "ostré retro pixely" (NEAREST). Při zvětšení 320×240 na
celou obrazovku to dělá přesně ty tvrdé kostky, co vidíš. Přepnuto na
hladké zvětšení (LINEAR) — hrany budou čisté.

## 2) Roztažené intro — zrada PlayStationu

V logu hra přepínala mezi 320×240, **512×480** a 640×480. PS1 ale nemá
čtvercové pixely! Nesmí se to roztahovat podle těch čísel, ale vždycky
na poměr **4:3**. Já to počítal z rozměrů — proto se 512×480 zúžilo.
Opraveno natvrdo na 4:3.

## 3) Trhání — moje brzda byla moc přísná

V logu: 300 snímků za 17–22 vteřin = **jen 15 FPS**. Kreslil jsem pouze
když jsem podle vzorku 64 bodů poznal změnu — a když se hýbal jen kousek
obrazu, změnu jsem přehlédl a snímek zahodil. Zrušeno, teď je jen
jednoduchý strop 60 FPS.

## 4) Bonus: tlačítko na logu = přepínač 4:3 ↔ 16:9

Obraz teď naskakuje sám, tak jsem tlačítku dal užitečnější práci:
klepnutím na logo NaP přepínáš mezi **4:3** (správné pro PS1) a
**16:9** (na televizi). Krátce se ukáže, co je zapnuté.

## 5) Názvy běhů v Actions

Opraveno — název běhu bude tvůj popisek z commitu, ať v tom máš pořádek.

## Verze: EMU10-E-HLADKY-OBRAZ-43

## Co mi pošli

- Je obraz hladší (bez kostiček)?
- Sedí teď poměr stran i u těch přepínajících inter?
- Kolik FPS ukazuje cast dole v liště?
- Zkus klepnout na logo — přepne se to na 16:9?

## Co bude dál

Zvuk mimo o 0,8 s a kvalita přenosu: to je ta cesta "vyfotit obrazovku
→ H264". Krok F bude tvůj nápad — posílat na TV rovnou z jádra
v čistém 16:9. Tím zmizí dvojí zvětšování i většina zpoždění.
