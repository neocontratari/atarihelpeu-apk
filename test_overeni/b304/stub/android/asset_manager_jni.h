// Nahrada <android/asset_manager_jni.h> pro preklad na pocitaci (B304).
#pragma once
#include <jni.h>
#include "asset_manager.h"
AAssetManager *AAssetManager_fromJava(JNIEnv *env, jobject assetManager);
