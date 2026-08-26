# B167 — KLÁVESNICE Z POČÍTAČE (versionCode 215)

## 1) Klávesnice — máš ji před srazem

V prohlížeči na počítači, kde se díváš na TV, **prostě píšeš**. Co
zmáčkneš, dostane Atari stejně, jako by ses toho dotkl na telefonu.

```
píšeš v prohlížeči  ->  /klavesa?code=KeyA&znak=a&dolu=1  ->  M.keyDown()
```

Vpravo dole se ukáže nápis `KLAVESNICE ZAPNUTA (F2 = BREAK)`.

Funguje písmena, číslice, Enter, mezerník, šipky, **Ctrl+klávesa**
i **BREAK přes F2**. `F5` a `F12` zůstávají prohlížeči, ať si můžeš
stránku obnovit.

### Bezpečnost — a chyba, kterou našel vlastní test

Klávesa jde z prohlížeče do stránky Atari jako kus JavaScriptu. Můj
první filtr **propouštěl apostrof a lomítka** a šlo tím podvrhnout
cizí kód. Test to našel na řetězci `'; alert(1); //`.

Teď projdou v názvu klávesy **jen písmena, číslice a podtržítko**,
a ve znaku neprojde apostrof, lomítko ani zpětné lomítko.

Ověřeno na osmi případech včetně pokusů o podvržení — **8/8 bezpečných**,
apostrofy v sestaveném kódu vždycky sedí.

## 2) Snímky z Atari se vůbec neodesílaly

V tvém logu z B165:

```
BUILD2SA51 ATARI_SNIMEK    0×
PIXELCOPY                704×  (avgLatencyMs=27)
```

Okno se snímalo dál. To zlepšení, cos viděl, přišlo z fronty na zvuk
z dřívějška — ne z toho, co jsem poslal.

**Proč:** stránka Atari běží z `file://` a posílá na `http://127.0.0.1`.
To je pro prohlížeč jiný původ a bez hlaviček CORS ten `POST` zablokuje.
Server je neposílal.

Doplněno, včetně odpovědi na předběžný dotaz `OPTIONS`.

A ještě jedna věc: server četl tělo `POST`u až po hlavičkách, **jenže
část těla přijde už v tom prvním čtení**. Bez toho by se první kus
snímku ztratil.

Když se snímek neodešle, Atari to teď **napíše do svého logu** — mlčky
se to už nestane.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS Atari i všech etap | 0 chyb |
| sestavený kód klávesy | 8/8 bezpečných |
| hlavičky CORS | doplněné |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Zapni WEB TV, otevři adresu v prohlížeči na počítači.
**3)** Jdi na telefonu do **ATARI 130XE**.
**4)** **Piš na klávesnici počítače — objevuje se to v Atari?**
**5)** Zkus `F2` (BREAK) a `Ctrl+písmeno`.
**6)** Kouše se ještě Atari?

## CO POSLAT ZPĚT

Log. Hledej:

```
BUILD2SA51 ATARI_SNIMEK 384x240 prijato=.. (bez PixelCopy)
```

Když tam bude, snímky konečně chodí a `PIXELCOPY` má být výrazně míň
než těch 704.
