# Co je v B324

Tvoje zpráva: „Ta aktualizace telefonu je obří, ale má chybičky – po odsouhlasení v appce to skáče
z appky ven pro další potvrzení – což by asi nevadilo – ale pak se to nesmyslně vrací zpět do appky
a stahuje to znova.“

## Proč to dělalo

- Po AKTUALIZOVAT se nová verze stáhne do instalace a Android si řekne o potvrzení (to „skočení ven“
  je správně – Android to chce).
- Android pak appce posílá zprávu, jak instalace dopadla. Appka si ji nechala doručit tak, že se
  **spustila znovu jako druhá kopie** nad tou první. Ta druhá kopie se po startu zeptala serveru,
  viděla novější verzi (stará se ještě nenainstalovala, Android čekal na potvrzení) a nabídla
  aktualizaci **znovu**. Odtud ten nesmyslný návrat a druhý dotaz.

## Oprava

- Zprávu od Androidu dostane **appka, která už běží** – žádná druhá kopie.
- Během stahování a instalace se aktualizace **znovu nenabízí ani nestahuje**.
- Po instalaci appka napíše „Instaluje se nová verze – appka se sama zavře a otevře znovu“ a v nové
  verzi „Aktualizace hotová – EMU10-B324…“.
- **Android 12 a novější:** appka, která se naposledy nainstalovala sama (aktualizací odsud), dostane
  další aktualizaci **bez toho potvrzování** – Android to tak umí. Starší Android se zeptá jako dřív.
- Nápověda (CZ i EN) to popisuje.

## Na rovinu

- **Přechod na B324 ještě udělá stará B323** – tahle jedna aktualizace se ještě může vrátit
  a nabídnout znovu. Na druhou nabídku klepni **TEĎ NE**. Od B324 dál už ne.
- Bez potvrzování to bude až u další aktualizace po B324 a jen na Androidu 12+.
- Ověřeno přeložením celé appky a kontrolou kódu (6 kontrol včetně logiky hlídání). Chování Androidu
  samotné se na PC pustit nedá – vyzkoušet na telefonu.
- Kdyby se appka po instalaci sama neotevřela (některé Androidy to po aktualizaci nedovolí), klepni
  na její ikonu – nová verze už tam je.
