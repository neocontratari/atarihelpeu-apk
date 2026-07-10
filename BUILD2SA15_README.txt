BUILD2SA15_STAGE203_ORIGINAL_SOUND_EVERYWHERE (nahrazuje SA14, zaklad RC10)

FILOZOFIE "ORIGINAL FIRST" TED PLATI VSUDE:
1) YOUTUBE (z SA14): zadny audio graf, dokud neotevres EQ panel. Do te
   doby hraje YouTube bit po bitu original. EQ = opt-in.
2) MP3 PREHRAVAC (novy fix): STEJNA nemoc - graf se stavel pri KAZDEM
   prehrani, MP3 uz nikdy nehralo ciste. Ted: play() zadny graf nestavi,
   MP3 hraje 100% original primo z <audio>. Graf se postavi az kdyz
   POPRVE sahnes na EQ/basy/vysky/balance. Log: "AUDIO GRAF: stavim az ted".
   TV odboc pro zrcadleni zustava (byl uz spravne mimo slysitelnou cestu).

ATARI - DECATHLON A ROZHOZENA GRAFIKA (dalsi krok, potrebuji tva data):
Emulator je skutecny (audit: taktove presny 6502), ale ma diry v grafickych
rezimech, ktere nektera hra pouziva (Decathlon = intenzivni player-missile
grafika + specialni rezimy). Presne jako u Segy: napred zmerit, pak riznout.
POSLI MI: 1) seznam her s rozbitou grafikou (nazvy staci)
2) z VBXE stranky uloz debug log pri spustenem Decathlonu
=> dostanes cileny fix rezimu, ne slepou berlicku.
Pozn: prejmenovani "fake-fast" -> TURBO_LOAD + prepinac autenticke
rychlosti pribalim do Atari fixu, at je to jedna Atari smena.

TEST: 1) MP3: pustit skladbu BEZ sahnuti na EQ - musi znit presne jako
v jinem prehravaci; pak pohnout basy - graf se postavi (log) a funguje.
2) YT: bez EQ = original. 3) Poslat Atari seznam + Decathlon log.

===== SA16 DOPLNKY (10.7.) =====
1) EQ PANEL CITELNY: plne nepruhledne pozadi (uz neprosvita YouTube),
   kazdy radek = popisek vlevo / HODNOTA vpravo s pevnou sirkou (nikdy
   se neusekne) / posuvnik pres celou sirku pod nimi. Vetsi pismo, ram.
2) ATARI SONDA HOTOVA (na zaklade tvych dvou her):
   - GRAFP/GRAFM (prime zapisy sprite registru) = 0 vyskytu v motoru
     -> NEIMPLEMENTOVANO. Presne typ diry, ktery odpovida "telo jinde
     nez nohy" (Decathlon) - stejny druh nalezu jako VDP blank u Segy.
   - HSCROL (jemny scroll pro "Welcome" text) existuje (35 vyskytu),
     ale Decathlon ho zjevne pouziva zpusobem, ktery ma diru - potrebuji
     debug log z bezici hry.
   - Popeye (chybi zebriky/plosiny pri viditelnych postavickach) =
     priznak diry v prioritach/hracim poli (PRIOR/CHBASE cesta).
   DALSI ATARI SMENA (rekni "jedem atari"): implementace GRAFP/GRAFM +
   VDELAY overeni + oprava rezimu podle debug logu z Decathlonu a Popeye
   + prejmenovani fake-fast -> TURBO_LOAD + prepinac autenticke rychlosti.
   Obe hry mam od tebe ulozene jako testovaci pripady.

===== SA17 DOPLNKY (finalni tohoto balicku) =====
1) YT ZVUK 1:1 GARANCE: v EQ panelu nove ZELENE tlacitko "1:1 ORIGINAL".
   Technicka pravda bez lakovani: jakmile se audio graf JEDNOU postavi
   (prvnim otevrenim EQ), prohlizec ho neumi "odpojit" - i v neutralu
   zustava zvuk routovany pres graf (temer, ale ne matematicky presne
   1:1). Tlacitko 1:1 ORIGINAL: vynuluje vse, ulozi a ZNOVU NACTE stranku
   -> graf je pryc, zvuk je garantovane bit-perfect original.
   A pokud EQ vubec neotevres, hraje original od zacatku (SA14/15).
2) ATARI - KOREKCE MEHO NALEZU (poctivost): GRAFP/GRAFM i VDELAY v motoru
   JSOU (muj vcerejsi grep mel chybny vzor - moje chyba). Registry se
   ukladaji; otazka je vykreslovaci cast. Motor uz ma zabudovany PM audit
   (BUILD2JT), ktery presne tohle meri.
   => PROSIM: spust Decathlon a pak Popeye, u kazdeho chvili hraj
   a uloz VBXE DEBUG LOG (oba posli). V logu budou BUILD2JT PM REG radky
   s GRACTL/DMACTL/GRAF stavem - ty mi reknou PRESNE, kterou cestou hry
   sprity kresli a kde je dira. Pak prijde chirurgicky fix render cesty
   + TURBO_LOAD prejmenovani + prepinac autenticke rychlosti - jedna
   Atari smena, podlozena tvymi logy. Obe hry mam jako testovaci pripady.
TEST: YT -> otevri EQ -> pohni posuvniky -> 1:1 ORIGINAL -> stranka se
znovu nacte a zvuk je presne jako v Chromu. MP3 stejne (bez EQ = original).
