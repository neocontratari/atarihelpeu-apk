# Co je ve v12 – registrace přímo v serveru N&P

Tvoje zpráva: „Chci registraci vestavět přímo do mého Universe Enginu … IP, e-mail, tabulka, schválit nebo
jedním klikem zablokovat, kód až po kontrole, děkovný e-mail s mým jménem“ + „veškeré tyto inovace nechám
na tobě – od tebe budu potřebovat návod“.

Appka se **nemění** (zůstává B325). Mění se server (v12, jen u tebe) a stránka 1130 na webu.

## 1. Jak to vidí člověk

1. Stránka 1130 → pod STÁHNOUT nové tlačítko **📝 Registrace / Register**.
2. Stránka tvého serveru: jméno nebo přezdívka, e-mail, souhlas → ZAREGISTROVAT (česky / anglicky).
3. Přijde e-mail od tebe („René Frank – N&P Vision“, podpis René Frank · RenatusHonda · N&P Vision ·
   atarihelp.eu): díky za registraci + tlačítko **POTVRDIT E-MAIL**.
4. Po potvrzení čeká na tebe. Po tvém **✔ SCHVÁLIT** mu přijde e-mail s osobním kódem a tlačítkem
   **STÁHNOUT APLIKACI** (stránka 1130 s vyplněným kódem).

## 2. Co hlídá server

- e-mail: jen běžné znaky, žádné jednorázové schránky (10 236 domén, měsíčně nový seznam), doména musí
  existovat a přijímat poštu (DNS),
- IP: 1 registrace z jedné sítě za 24 h, starší registrace ze stejné IP = varování v tabulce,
- stejný e-mail podruhé neprojde (ani Gmail s tečkami / +něco), roboti nic neuloží ani nepošlou,
- zablokovaný e-mail / IP: stránka řekne „Děkujeme“, ale nic se neuloží ani nepošle,
- země podle IP (DB-IP Lite, licence CC BY 4.0 – proto je na stránce registrace odkaz na db-ip.com).

## 3. Stránka 1130 (web/stranka_1130/html_kod_www_stranky_v12.txt)

- Tlačítko REGISTRACE vede na server (adresa je ve skriptu i u tlačítka).
- Odkazy z e-mailů vedou přes tuhle stránku (https, méně spamu): `&nap_potvrdit=…` otevře potvrzení na
  serveru, `&nap_kod=…` otevře okno kódu s vyplněným kódem.
- Pravidla pro WordPress dodržená (žádný ampersand ani znak menší ve skriptu atd.).

## Na rovinu

- **Ověřeno na PC:** stránka v Chromiu tak, jak ji pošle WordPress (surová, Gutenberg, klasický editor,
  odložený JavaScript): registrace 42 kontrol, WordPress 12, stará kontrola stránky 16. Server u tebe:
  314 kontrol (100 nových s falešným poštovním serverem a DNS) + okna serveru.
- **Se skutečným Seznamem a tvým webem ne** – návod NAVOD_SERVER_v12.txt (v balíčku serveru) má krok
  „Vyzkoušej to sám“.
- Server běží přes http (ne https) – jméno a e-mail jdou sítí nešifrovaně, stejně jako dosud zadání kódu.
