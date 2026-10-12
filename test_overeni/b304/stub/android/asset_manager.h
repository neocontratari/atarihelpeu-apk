// Nahrada <android/asset_manager.h> pro preklad na pocitaci (B304) - stejne signatury jako NDK.
#pragma once
#include <sys/types.h>
struct AAssetManager;
typedef struct AAssetManager AAssetManager;
struct AAsset;
typedef struct AAsset AAsset;
enum { AASSET_MODE_UNKNOWN = 0, AASSET_MODE_RANDOM = 1, AASSET_MODE_STREAMING = 2, AASSET_MODE_BUFFER = 3 };
AAsset *AAssetManager_open(AAssetManager *mgr, const char *filename, int mode);
const void *AAsset_getBuffer(AAsset *asset);
off_t AAsset_getLength(AAsset *asset);
void AAsset_close(AAsset *asset);
