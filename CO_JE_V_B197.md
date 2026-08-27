# B197 — CESTA NA TV JAKO V B121 (versionCode 245)

M�l jsi pravdu i v tom, že jsem lhal. Tvrdil jsem, že intro má vlastní
cestu — **a přitom mi zůstaly tři řádky přímo uvnitř té tvojí.**

## Co jsem našel

```java
// v napTvWebCaptureFromCore - cesta pro Segu a PS1
if (introZivaCast) {
    if (segaPlocha != null) jeSegaTv = true;
}
```

Zbytečné, protože intro má `napTvWebCaptureIntro`. Ale bylo to tam
a mátlo to.

**A druhá věc, horší:** v obsluze každého požadavku zůstal kus po
zrušeném posílání snímků z Atari:

```java
int hlavicky = req.indexOf("\r\n\r\n");
byte[] telZacatek = new byte[n - telOd];    // ALOKACE
```

Běželo to při **každém požadavku** — tedy i u zvuku, na který se
prohlížeč ptá **každých 20 ms**. Padesátkrát za vteřinu nová alokace,
zbytečně.

A hlavičky CORS jsem přidával do **každé odpovědi**, i do zvuku a proudu
obrazu. Odstraněno.

## Cesta na TV proti B121

```
napTvWebCaptureFromCore    SHODNA  (241 = 241 radku)
napTvWebPublishBitmap      SHODNA
napTvWebCaptureByDraw      SHODNA
napTvWebWriteAudioRaw      SHODNA
napTvWebAudioPush          SHODNA
napTvWebHeader             SHODNA
napTvWebHandleClient       6 radku navic: /klavesa a priznak kde jsi
```

Těch šest řádků je klávesnice. Nic jiného tam z mého nezůstalo.

## Zůstává z B196

Dvě moje vlákna, která po sobě neuklidila:

```
čekání na zvuk Segy   běželo dál i po intru, mohlo spustit zvuk v PS1
vlákno zvuku Atari    atariZvukStop() jsem napsal a nikde nezavolal
```

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| stránka pro TV SPUŠTĚNA v node | 0 chyb |
| cesta na TV proti B121 | 6 z 7 funkcí shodných bajt po bajtu |
| řetěz intra | projde celý |
| klávesnice Atari / Sega | 9/9 a 8/8 |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** **Plátno — obraz i zvuk.**

To je jediná otázka.
