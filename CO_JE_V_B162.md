# B162 — MOST UŽ NEČEKÁ (versionCode 210)

M�l jsi pravdu ve všem. Snímání to nebylo a osekávat obraz na TV nemělo
smysl — vrátil jsem to.

## Co změření ukázalo

Z tvého logu:

```
kopieBitmapy=150ms  pixely=242ms  drain=89ms     za 2,5 s
```

To je asi **19 %** hlavního vlákna, ne 141 %, jak jsem počítal z odhadu.
Snímání tedy **není příčina** a moje osekávání bylo špatně.

## Kde to je doopravdy

Atari posílá zvuk na TV **z JavaScriptu** — a ta cesta vypadá takhle:

```java
@JavascriptInterface public String pushAtariPcm16(String b64, ...) {
    byte[] data = Base64.decode(b64, ...);
    napTvWebAudioPushMonoPcm16Bytes(data, ...);   // ← a tady
}
```

A uvnitř té poslední funkce:

```java
synchronized (napTvWebAudioLock) {
    for (int i = 0; i < len; i += 2) {
        ... napTvWebAudioSeq % cap ...   // čtyři dělení na každý vzorek
    }
}
```

**Javascriptové vlákno — to samé, na kterém běží emulátor — čekalo na
ten zámek.** A o něj se pere s vláknem, které posílá zvuk na televizi.

Proto se to kouše **i na mobilu** a jen když je TV zapnutá.

Samotné rozkódování base64 je přitom levné — změřil jsem to, 0,3 ms za
vteřinu. Nešlo o dekódování, ale o to čekání.

## Oprava

Dávka se **jen odloží do fronty a odpoví se hned**. Rozkódování i předání
dělá vlastní vlákno, takže emulátor už nikde nečeká.

Fronta je záměrně krátká (8 dávek). Když se nestíhá, zahodí se nejstarší
a zapíše se to do logu — lepší krátký výpadek zvuku než rostoucí zpoždění.

## Čeho jsem se nedotkl

Převod na text a `btoa()` **na straně prohlížeče zůstává** — to je uvnitř
původního Atari a bez tvého svolení na to nesahám. Je to dalších asi 33 %
dat navíc a nějaký čas v JS.

Ověřeno: `assets/emu_vbxe` je proti B123 **bajt po bajtu netknuté**.

## A přidal jsem měření

```
BUILD2SA48 ATARI_MOST prumer=..us davek=.. fronta=..
```

Řekne, jak dlouho ten most opravdu drží JS vlákno. **Příště to nebudu
odhadovat.**

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS všech etap | 0 chyb |
| Java Atari (`emu_vbxe`) | netknuté proti B123 |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Zapni WEB TV, jdi do **ATARI 130XE**, zahraj si.
**3)** **Kouše se obraz nebo zvuk — na mobilu nebo na TV?**
**4)** Obraz na TV má být zase v plném rozlišení.

## CO POSLAT ZPĚT

Log. Hledej:

```
BUILD2SA48 ATARI_MOST prumer=..us davek=.. fronta=..
BUILD2SA48 ATARI_ZVUK zahozeno davek=..
```

**Ten první řádek je klíčový.** Když bude průměr vysoký (stovky
mikrosekund a víc), most pořád drží emulátor a půjdu dál. Když bude
nízký a přesto se to kouše, je příčina na straně prohlížeče — a to už
je uvnitř Atari, kam bez tvého svolení nesahám.
