#pragma once
#include "OpenSLES.h"
struct SLAndroidSimpleBufferQueueItf_; typedef const struct SLAndroidSimpleBufferQueueItf_ *const *SLAndroidSimpleBufferQueueItf;
typedef void (*slAndroidSimpleBufferQueueCallback)(SLAndroidSimpleBufferQueueItf caller, void *pContext);
struct SLAndroidSimpleBufferQueueItf_ {
  SLresult (*Enqueue)(SLAndroidSimpleBufferQueueItf self, const void *pBuffer, SLuint32 size);
  SLresult (*RegisterCallback)(SLAndroidSimpleBufferQueueItf self, slAndroidSimpleBufferQueueCallback cb, void *ctx);
};
typedef struct SLDataLocator_AndroidSimpleBufferQueue { SLuint32 locatorType; SLuint32 numBuffers; } SLDataLocator_AndroidSimpleBufferQueue;
#define SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE 0x800007BD
extern const SLInterfaceID SL_IID_ANDROIDSIMPLEBUFFERQUEUE;
