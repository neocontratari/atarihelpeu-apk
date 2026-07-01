BUILD2RM_SEGA_NATIVE_CPP_ONLY_ANTI_ALIAS_AUDIO_FRONTEND_STAGE129

UCEL:
- Opravny build po RL/RK: ne dalsi ztiseni hudby, ale odstraneni fake bass/gong artefaktu z audio frontendu.
- Core zustava ClownMDEmu C++ only, Java/WebView Sega wrapper zustava vypnuty.

ZMENY:
- FM/YM2612 a PSG se uz neresampluji vyberem jednoho vzorku.
- PSG native ~223 kHz -> 48 kHz jde pres box-average anti-alias downsampler.
- FM native ~53 kHz -> 48 kHz jde pres box-average downsampler.
- Odstraneno sample-pick aliasovani, ktere mohlo delat falesny hluboky gong/bass rumble.
- FM neni jen ztiseny: fmGain=64, psgGain=56, masterGain=92.
- Jemny DC blocker pouze proti DC/thump, ne bass clamp.
- Start audio ma 4096-frame ramp proti uvodnimu gongu/thumpu.
- LOW/HIGH nemeni audio clock ani gain.

TEST:
1) Workflow musi projit.
2) Testuj jen Nox + Sonic intro.
3) Sleduj: uvodni gong, basy, SEGA hlas/napisy, pilotni napev, zpomaleni hudby.
4) Mobil zatim netestovat, dokud nebude Nox zvuk cisty.

LOG MARKERY:
- BUILD2RM_SEGA_NATIVE_CPP_ONLY_ANTI_ALIAS_AUDIO_FRONTEND_STAGE129
- FM_PSG_ANTIALIAS_FRONTEND_RM
- boxAverageResampler=YES
- antiAliasPSG=YES
- noSamplePick=YES
- startupRamp=4096
- fmGain=64 psgGain=56 masterGain=92
- NATIVE_AUDIO_STREAM_START_RM_ANTIALIAS_AUDIO_QT
- NATIVE_AUDIO_PULL_RM_ANTIALIAS_AUDIO_QT
