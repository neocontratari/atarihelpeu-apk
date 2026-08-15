# B102 — PLNÝ BAREVNÝ ROZSAH (versionCode 150)

> **Tento kód je předpoklad. Čeká se na test.**
> Sáhl jsem **jen na nastavení enkodéru pro TV**.

## Ty čtyři obrázky prozradily příčinu

Podívej se na ně znovu: **na tmavém pozadí u NFS a Dolby jsou kostky, ale
nápisy MEMORY CARD a CD PLAYER jsou úplně ostré.**

Kdyby byl málo datový tok, rozpadlo by se **všechno**. Rozpadá se jen jemné
stínování v tmavých plochách — a to má jinou příčinu.

## Enkodér stlačoval jas

H.264 má ve výchozím stavu **„televizní" rozsah**: jas jen 16 až 235 místo
0 až 255. Obraz z PlayStation má plný rozsah, takže se stlačil do užšího —
a v tmavých plochách zbylo **míň úrovní na jemné stínování**. Odtud kostky.

Ostré nápisy jsou v pořádku, protože mají plný kontrast a rozsah jim nevadí.
Přesně to je na tvých obrázcích.

Teď je nastavený plný rozsah. **Tohle byla ta hlavní příčina, ne datový tok.**

## Plus dvě věci

**Profil High** místo Main — umí jemnější přechody, což je přesně to, co
tmavé plochy potřebují.

**Opravený log.** Řádek `H264_ENCODER_START` hlásil `bitrate=5529600`, tedy
5,5 Mbit/s — jenže to bylo jen **špatně spočítané číslo ve výpisu**, ve
skutečnosti běželo 27 Mbit/s. Log lhal a málem mě to poslalo špatným směrem.
Opraveno, aby hlásil skutečnou hodnotu.

## Odpověď na tvou otázku, jestli to jde udělat čistě

**Jde.** Není to nic nemožného ani žádný fake. Obraz jde z jádra do enkodéru
v plné kvalitě a jediné, co ho kazilo, bylo nastavení enkodéru — tedy to,
jak se ten hotový obraz zabalí pro přenos. Nic se nemusí dopočítávat ani
vyhlazovat.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64, skutečné `jni.h` | 0 chyb |
| symboly | `nm -u` proti `nm --defined-only` | všechny sedí |

---

## CO TESTOVAT

**1)** Zapni TV a pusť **NFS** — to intro s auty
**2)** Sleduj **Dolby logo** na začátku (černé pozadí)
**3)** Zkontroluj, že se mobil nezhoršil

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | kostky v tmavých plochách **výrazně míň nebo pryč** | stejné |
| 2 | černá je hladká, bez čtverců kolem loga | pořád čtverce |
| 3 | mobil beze změny | zhoršilo se |

Kdyby byl obraz na TV najednou **moc kontrastní nebo vybledlý**, znamená to,
že prohlížeč ten plný rozsah nečeká — řekni a nastavím to zpátky. Poznáš to
hned, není to nic nenápadného.

## CO POSLAT ZPĚT

Jsou kostky v tmavých plochách pryč? A nezměnily se barvy?
