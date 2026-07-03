// BUILD2SA1: PS1 core bridge - POUZE dukaz zivota jadra (retro_get_system_info).
// Zadny fake boot. BIOS/CD boot prijde v SA2/SA3 pres stejnou disciplinu jako Sega.
#include <jni.h>
#include <string.h>
#include <stdio.h>
extern "C" {
  unsigned retro_api_version(void);
  struct retro_system_info { const char *library_name; const char *library_version; const char *valid_extensions; unsigned char need_fullpath; unsigned char block_extract; };
  void retro_get_system_info(struct retro_system_info *info);
}
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1CoreInfo(JNIEnv *env, jclass) {
  char buf[512];
  struct retro_system_info si; memset(&si,0,sizeof(si));
  retro_get_system_info(&si);
  snprintf(buf,sizeof(buf),"PS1_CORE_COMPILED name=%s version=%s api=%u ext=%s dynarec=INTERPRETER_SA1 boot=NOT_YET_SA2",
    si.library_name?si.library_name:"?", si.library_version?si.library_version:"?",
    retro_api_version(), si.valid_extensions?si.valid_extensions:"?");
  return env->NewStringUTF(buf);
}
