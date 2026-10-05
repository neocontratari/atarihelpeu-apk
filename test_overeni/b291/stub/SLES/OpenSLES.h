// Minimalni nahrada OpenSL ES pro test na pocitaci (jen co pouziva nap_atari_native.cpp).
#pragma once
#include <cstdint>
typedef uint32_t SLuint32; typedef SLuint32 SLresult; typedef SLuint32 SLboolean;
#define SL_RESULT_SUCCESS 0
#define SL_BOOLEAN_FALSE 0
#define SL_BOOLEAN_TRUE 1
struct SLInterfaceID_ { int id; };
typedef const struct SLInterfaceID_ *SLInterfaceID;
struct SLObjectItf_; typedef const struct SLObjectItf_ *const *SLObjectItf;
struct SLObjectItf_ {
  SLresult (*Realize)(SLObjectItf self, SLboolean async);
  SLresult (*GetInterface)(SLObjectItf self, const SLInterfaceID iid, void *pInterface);
  void (*Destroy)(SLObjectItf self);
};
typedef struct SLDataSource_ { void *pLocator; void *pFormat; } SLDataSource;
typedef struct SLDataSink_ { void *pLocator; void *pFormat; } SLDataSink;
struct SLEngineItf_; typedef const struct SLEngineItf_ *const *SLEngineItf;
struct SLEngineItf_ {
  SLresult (*CreateAudioPlayer)(SLEngineItf self, SLObjectItf *pPlayer, SLDataSource *src, SLDataSink *snk, SLuint32 n, const SLInterfaceID *ids, const SLboolean *req);
  SLresult (*CreateOutputMix)(SLEngineItf self, SLObjectItf *pMix, SLuint32 n, const SLInterfaceID *ids, const SLboolean *req);
};
struct SLPlayItf_; typedef const struct SLPlayItf_ *const *SLPlayItf;
struct SLPlayItf_ { SLresult (*SetPlayState)(SLPlayItf self, SLuint32 state); };
#define SL_PLAYSTATE_PLAYING 3
typedef struct SLDataLocator_OutputMix_ { SLuint32 locatorType; SLObjectItf outputMix; } SLDataLocator_OutputMix;
typedef struct SLDataFormat_PCM_ { SLuint32 formatType, numChannels, samplesPerSec, bitsPerSample, containerSize, channelMask, endianness; } SLDataFormat_PCM;
#define SL_DATAFORMAT_PCM 2
#define SL_SAMPLINGRATE_44_1 44100000
#define SL_PCMSAMPLEFORMAT_FIXED_16 16
#define SL_SPEAKER_FRONT_LEFT 1
#define SL_SPEAKER_FRONT_RIGHT 2
#define SL_BYTEORDER_LITTLEENDIAN 2
#define SL_DATALOCATOR_OUTPUTMIX 4
extern const SLInterfaceID SL_IID_ENGINE;
extern const SLInterfaceID SL_IID_PLAY;
SLresult slCreateEngine(SLObjectItf *pEngine, SLuint32 numOptions, const void *opts, SLuint32 numInterfaces, const SLInterfaceID *ids, const SLboolean *req);
