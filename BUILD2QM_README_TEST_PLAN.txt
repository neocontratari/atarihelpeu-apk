BUILD2QM_SEGA_NATIVE_CPP_AUDIO_STABLE_FM_LANDSCAPE_SAFE_STAGE103

KODY JSOU ZMENENE: ANO

CIL:
- navazuje na 2QK/2QL real C++ Sega core
- 2QL mel mensi prodlevu, ale zvuk na Noxu chrci jeste vic
- 2QM proto vraci audio do stabilnejsi FM-only cesty, PSG/PCM zatim jen diagnostika
- zachova realny native render a region auto
- fixuje landscape C++ view tak, aby neprekrýval joystick/tlacitka

TEST:
1. SEGA -> C++ CORE
2. Sonic v Noxu
3. Sleduj: hudba, cinknuti prstynku, skok, chrceni, prodleva
4. Zkus landscape, jestli je videt joystick
5. ULOZENE a poslat log

CEKAM V LOGU:
BUILD2QM_SEGA_NATIVE_CPP_AUDIO_STABLE_FM_LANDSCAPE_SAFE_STAGE103
NATIVE_AUDIO_STREAM_START_STABLE_FM
audio_mode=STABLE_FM_ONLY_QM
psg_seen_not_mixed=YES
landscapeSafeControls=YES pri landscape

POZNAMKA:
Tohle neni final audio. Je to stabilizacni rollback po spatnem 2QL mixeru.
