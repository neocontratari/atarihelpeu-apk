# KROK F3 — zrychlení přenosu

## Co jsem opravil

**1) Nízké FPS (20 na HIGH) — moje neopatrnost.**
V nové funkci jsem před každým snímkem projížděl **všech ~307 000
pixelů** jen kvůli jedné drobnosti s průhledností. Šedesátkrát za
vteřinu. To samé teď zařídí jeden příkaz místo miliónů kroků.
Odstraněno i zbytečné vyhlazování hran, které u zvětšování obrazu
stejně nic nedělá.

**2) Výpadky na 0 FPS na začátku.**
Když jádro zrovna nemá snímek (typicky při načítání hry), propadalo to
zpátky na fotografování obrazovky → černo a nula. Teď se místo toho
podrží poslední dobrý snímek.

## Verze: EMU10-F3-RYCHLEJSI-TV

## Co mi pošli

- Kolik FPS teď ukazuje cast na HIGH?
- Zmizely ty výpadky na 0 na začátku?
- Vypadá obraz pořád stejně dobře?
