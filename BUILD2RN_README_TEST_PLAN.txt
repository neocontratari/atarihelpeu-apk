BUILD2RN_SEGA_NATIVE_CPP_ONLY_REFERENCE_AUDIO_CLEANUP_STAGE130

UCEL:
- Opravny build po RM: ne dalsi zeslabovani, ale realny zasah do pricin bass/gong bordelu.
- Core zustava ClownMDEmu C++ only, Java/WebView Sega wrapper zustava vypnuty.

ZMENY:
- Core VA4 low-pass je vypnuty: g_real.cfg.general.low_pass_filter_disabled = cc_true.
  Duvod: Android/Nox vystup byl prebasovany a Sonic/SEGA lead/pilotni napev byl utlumeny.
- FM/YM2612 stereo zustava zachovane, FM uz neni jen tise stazene.
- FM native ~53 kHz -> 48 kHz pres box-average downsampler.
- PSG native ~223 kHz -> 48 kHz pres box-average downsampler.
- Final output ma rumble blocker proti falešnemu hlubokemu gong/bass artefaktu.
- Start audio ma kratky mute 2048 output frames proti reset/thump/gong na zacatku.
- Android AudioTrack je stale stereo 48 kHz, ale audio attribute uz neni MUSIC bass-enhancement cesta.
- LOW/HIGH nemeni audio clock ani gain.

TEST:
1) Workflow musi projit.
2) Testuj jen Nox + Sonic intro.
3) Sleduj: uvodni gong, basy, SEGA hlas/napisy, pilotni napev, zpomaleni hudby.
4) Mobil zatim netestovat, dokud nebude Nox zvuk cisty.

LOG MARKERY:
- BUILD2RN_SEGA_NATIVE_CPP_ONLY_REFERENCE_AUDIO_CLEANUP_STAGE130
- FM_PSG_REFERENCE_FRONTEND_RN
- noCoreLPF=YES
- rumbleBlock=YES
- boxAverageResampler=YES
- startupMute=2048
- fmGain=100 psgGain=48 masterGain=96
- NATIVE_AUDIO_STREAM_START_RN_REFERENCE_AUDIO_QT
- NATIVE_AUDIO_PULL_RN_REFERENCE_AUDIO_QT
