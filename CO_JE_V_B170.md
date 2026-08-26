# B170 — KLÁVESNICE (versionCode 218)

Tvoje poznámka byla přesná: *„nemáš zpětnou vazbu — apka vůbec nezná,
co se děje."* Přesně tam byla chyba.

## Co bylo špatně

`CODE` a `M` **žijí uvnitř funkce** ve stránce Atari — zvenku k nim
nejde. Java tedy skládala kus JavaScriptu, který na ně sahal, a dostala
chybu.

A protože jsem předával `null` místo zpětné vazby, **ta chyba se ztratila**.
Aplikace o ní nevěděla, log mlčel a já pak hádal.

## Jak to je teď

Stránka Atari vystavuje **jeden vstupní bod** — uvnitř té funkce, kde
`CODE` i `M` bydlí:

```js
window.napKlavesa = function(code, znak, dolu, ctrl){
  ...
  return 'OK:'+code+'=scan'+(sc&0xFF);
};
```

Vrací řetězec a **Java ho poslouchá**:

```
BUILD2SA56 KLAVESA KeyA dolu -> OK:KeyA=scan63 (celkem 1)
BUILD2SA56 KLAVESA F7 dolu -> NEZNAMA:F7
```

První klávesa a každá chyba se hlásí vždycky, ostatní nejvýš jednou
za tři vteřiny, aby log nezaplnily.

## Ověřeno spuštěním, ne odhadem

Pustil jsem tu funkci **proti skutečné tabulce kláves z Atari**:

```
KeyA        -> OK:KeyA=scan63       keyDown(scan 63)
Digit1      -> OK:Digit1=scan31     keyDown(scan 31)
Enter       -> OK:Enter=scan12      keyDown(scan 12)
Space       -> OK:Space=scan33      keyDown(scan 33)
KeyC+Ctrl   -> OK:KeyC=scan146      keyDown(scan 146)
Break       -> BREAK                breakKey()
Period      -> OK:Period=scan34     keyDown(scan 34)
ArrowUp     -> OK:ArrowUp=scan142   keyDown(scan 142)
pusteni     -> PUSTENO              keyUp()

došlo do emulátoru: 9 z 10
```

To desáté je `F7`, které Atari nemá — a správně hlásí `NEZNAMA`.

## Zůstává z B169

Snímek z Atari **se vyzvedává smyčkou TV**, přesně jako u Segy —
dřív jsem ho cpal z HTTP vlákna, odtud zpoždění 5-7 s a zamrzlý obraz.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS Atari i všech etap | 0 chyb |
| klávesy proti pravé tabulce | 9/10 (10. Atari nemá) |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Zapni WEB TV, otevři adresu v prohlížeči na počítači.
**3)** Jdi na telefonu do **ATARI 130XE**.
**4)** **Piš na klávesnici počítače** — píše to do Atari?
**5)** Zkus `F2` (BREAK) a `Ctrl+písmeno`.
**6)** Kouše se ještě? Je zpoždění?

## CO POSLAT ZPĚT

Log. Teď už **vždycky uvidíš, co se stalo**:

```
BUILD2SA56 KLAVESA KeyA dolu -> OK:KeyA=scan63
```

| co uvidíš | co to znamená |
|---|---|
| `OK:...=scan..` | klávesa došla až do emulátoru |
| `NENI_NA_STRANCE` | nejsi v Atari nebo se stránka nenačetla |
| `NEZNAMA:...` | Atari tu klávesu nemá |
| `CHYBA:...` | řekne přesně co |
| **nic** | klávesa se z prohlížeče neodeslala |

Ať to dopadne jakkoli, **už nebudeme hádat**.
