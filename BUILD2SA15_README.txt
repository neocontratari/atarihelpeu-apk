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
