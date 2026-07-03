// BUILD2SA2: PS1 libretro host - REALNY boot BIOS+hra, worker s retro_run,
// zachytavani snimku do bufferu, status s pocitadlem snimku. Obraz na
// TextureView je SA2b; zvuk SA3. Zadny fake - kdyz boot selze, status to prizna.
#include <jni.h>
#include <android/log.h>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#define NAPLOG(...) __android_log_print(ANDROID_LOG_INFO, "NAP_PS1", __VA_ARGS__)

extern "C" {
  // minimalni libretro API (shodne s libretro.h ABI)
  struct retro_system_info { const char *library_name, *library_version, *valid_extensions; unsigned char need_fullpath, block_extract; };
  struct retro_game_geometry { unsigned base_width, base_height, max_width, max_height; float aspect_ratio; };
  struct retro_system_timing { double fps, sample_rate; };
  struct retro_system_av_info { struct retro_game_geometry geometry; struct retro_system_timing timing; };
  struct retro_game_info { const char *path; const void *data; size_t size; const char *meta; };
  struct retro_variable { const char *key; const char *value; };
  struct retro_log_callback { void (*log)(int level, const char *fmt, ...); };
  typedef bool (*retro_environment_t)(unsigned cmd, void *data);
  typedef void (*retro_video_refresh_t)(const void *data, unsigned width, unsigned height, size_t pitch);
  typedef void (*retro_audio_sample_t)(int16_t left, int16_t right);
  typedef size_t (*retro_audio_sample_batch_t)(const int16_t *data, size_t frames);
  typedef void (*retro_input_poll_t)(void);
  typedef int16_t (*retro_input_state_t)(unsigned port, unsigned device, unsigned index, unsigned id);
  void retro_set_environment(retro_environment_t);
  void retro_set_video_refresh(retro_video_refresh_t);
  void retro_set_audio_sample(retro_audio_sample_t);
  void retro_set_audio_sample_batch(retro_audio_sample_batch_t);
  void retro_set_input_poll(retro_input_poll_t);
  void retro_set_input_state(retro_input_state_t);
  void retro_init(void);
  void retro_deinit(void);
  unsigned retro_api_version(void);
  void retro_get_system_info(struct retro_system_info*);
  void retro_get_system_av_info(struct retro_system_av_info*);
  bool retro_load_game(const struct retro_game_info*);
  void retro_unload_game(void);
  void retro_run(void);
}
#define ENV_SET_PIXEL_FORMAT 10
#define ENV_GET_SYSTEM_DIRECTORY 9
#define ENV_GET_SAVE_DIRECTORY 31
#define ENV_GET_CAN_DUPE 3
#define ENV_GET_LOG_INTERFACE 27
#define ENV_GET_VARIABLE 15
#define ENV_GET_VARIABLE_UPDATE 17
#define PIXFMT_0RGB1555 0
#define PIXFMT_XRGB8888 1
#define PIXFMT_RGB565 2

static std::string g_sysdir, g_savedir, g_boot_error;
static std::atomic<int> g_pixfmt{PIXFMT_0RGB1555};
static std::atomic<bool> g_running{false};
static std::atomic<int> g_generation{0};
static std::atomic<uint64_t> g_frames{0}, g_dupe_frames{0}, g_audio_samples_dropped{0};
static std::atomic<int> g_fw{0}, g_fh{0};
static std::mutex g_frame_mutex;
static std::vector<uint32_t> g_frame_argb; // SA2b si tenhle buffer vyzvedne pro TextureView
static std::thread g_worker;
static std::mutex g_life_mutex;
static double g_fps = 60.0;
static std::atomic<bool> g_loaded{false};

static void nap_retro_log(int level, const char *fmt, ...) {
  char buf[512]; va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
  __android_log_print(ANDROID_LOG_INFO, "NAP_PS1_CORE", "[%d] %s", level, buf);
}
static bool nap_env(unsigned cmd, void *data) {
  switch (cmd) {
    case ENV_SET_PIXEL_FORMAT: { g_pixfmt.store(*(const int*)data); return true; }
    case ENV_GET_SYSTEM_DIRECTORY: { *(const char**)data = g_sysdir.c_str(); return true; }
    case ENV_GET_SAVE_DIRECTORY: { *(const char**)data = g_savedir.c_str(); return true; }
    case ENV_GET_CAN_DUPE: { *(bool*)data = true; return true; }
    case ENV_GET_LOG_INTERFACE: { ((retro_log_callback*)data)->log = nap_retro_log; return true; }
    case ENV_GET_VARIABLE: { ((retro_variable*)data)->value = nullptr; return false; } // core defaults
    case ENV_GET_VARIABLE_UPDATE: { *(bool*)data = false; return true; }
    default: return false;
  }
}
static void nap_video(const void *data, unsigned w, unsigned h, size_t pitch) {
  if (!data) { g_dupe_frames.fetch_add(1); g_frames.fetch_add(1); return; }
  std::lock_guard<std::mutex> lock(g_frame_mutex);
  g_frame_argb.resize((size_t)w * h);
  const int fmt = g_pixfmt.load();
  for (unsigned y = 0; y < h; ++y) {
    const uint8_t *row = (const uint8_t*)data + y * pitch;
    uint32_t *dst = g_frame_argb.data() + (size_t)y * w;
    if (fmt == PIXFMT_RGB565) {
      const uint16_t *s = (const uint16_t*)row;
      for (unsigned x = 0; x < w; ++x) { uint16_t c = s[x];
        uint32_t r=(c>>11)&31,g=(c>>5)&63,b=c&31;
        dst[x]=0xFF000000u|((r<<3|r>>2)<<16)|((g<<2|g>>4)<<8)|(b<<3|b>>2); }
    } else if (fmt == PIXFMT_XRGB8888) {
      const uint32_t *s = (const uint32_t*)row;
      for (unsigned x = 0; x < w; ++x) dst[x] = 0xFF000000u | s[x];
    } else { // 0RGB1555
      const uint16_t *s = (const uint16_t*)row;
      for (unsigned x = 0; x < w; ++x) { uint16_t c = s[x];
        uint32_t r=(c>>10)&31,g=(c>>5)&31,b=c&31;
        dst[x]=0xFF000000u|((r<<3|r>>2)<<16)|((g<<3|g>>2)<<8)|(b<<3|b>>2); }
    }
  }
  g_fw.store((int)w); g_fh.store((int)h);
  g_frames.fetch_add(1);
}
// BUILD2SA3: audio FIFO (44100 Hz stereo z jadra -> Java AudioTrack)
static std::mutex g_amutex;
static std::vector<int16_t> g_afifo; // interleaved L,R
static const size_t NAP_PS1_AFIFO_MAX = 44100 * 2 * 2; // max 2 s, pak zahazujeme nejstarsi
static void nap_audio_push(const int16_t *data, size_t frames) {
  if (!data || !frames) return;
  std::lock_guard<std::mutex> lock(g_amutex);
  g_afifo.insert(g_afifo.end(), data, data + frames * 2);
  if (g_afifo.size() > NAP_PS1_AFIFO_MAX) {
    g_audio_samples_dropped.fetch_add((g_afifo.size() - NAP_PS1_AFIFO_MAX) / 2);
    g_afifo.erase(g_afifo.begin(), g_afifo.begin() + (g_afifo.size() - NAP_PS1_AFIFO_MAX));
  }
}
static void nap_audio_sample(int16_t l, int16_t r) { int16_t s[2] = { l, r }; nap_audio_push(s, 1); }
static size_t nap_audio_batch(const int16_t *data, size_t frames) { nap_audio_push(data, frames); return frames; }
static void nap_input_poll(void) {}
static int16_t nap_input_state(unsigned, unsigned, unsigned, unsigned) { return 0; } // SA3: realny vstup

static void nap_worker(int gen) {
  NAPLOG("BUILD2SA2 PS1 worker start gen=%d fps=%.2f", gen, g_fps);
  const auto period = std::chrono::nanoseconds((long long)(1e9 / (g_fps > 1 ? g_fps : 60.0)));
  auto next = std::chrono::steady_clock::now();
  while (g_running.load() && gen == g_generation.load()) {
    retro_run();
    next += period;
    auto now = std::chrono::steady_clock::now();
    if (next > now) std::this_thread::sleep_for(next - now); else next = now;
  }
  NAPLOG("BUILD2SA2 PS1 worker stop gen=%d frames=%llu", gen, (unsigned long long)g_frames.load());
}

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1CoreInfo(JNIEnv *env, jclass) {
  char buf[512]; retro_system_info si; memset(&si,0,sizeof(si)); retro_get_system_info(&si);
  snprintf(buf,sizeof(buf),"PS1_CORE_COMPILED name=%s version=%s api=%u ext=%s dynarec=INTERPRETER boot=SA2_READY",
    si.library_name?si.library_name:"?", si.library_version?si.library_version:"?", retro_api_version(), si.valid_extensions?si.valid_extensions:"?");
  return env->NewStringUTF(buf);
}
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1Boot(JNIEnv *env, jclass, jstring jsys, jstring jsave, jstring jgame) {
  std::lock_guard<std::mutex> life(g_life_mutex);
  char out[768];
  const char *sys = env->GetStringUTFChars(jsys, nullptr);
  const char *sav = env->GetStringUTFChars(jsave, nullptr);
  const char *game = env->GetStringUTFChars(jgame, nullptr);
  std::string gamePath = game ? game : "";
  g_sysdir = sys ? sys : ""; g_savedir = sav ? sav : "";
  env->ReleaseStringUTFChars(jsys, sys); env->ReleaseStringUTFChars(jsave, sav); env->ReleaseStringUTFChars(jgame, game);
  // stop pripadneho predchoziho behu
  g_generation.fetch_add(1); g_running.store(false);
  if (g_worker.joinable()) g_worker.join();
  if (g_loaded.exchange(false)) { retro_unload_game(); retro_deinit(); }
  g_frames.store(0); g_dupe_frames.store(0); g_audio_samples_dropped.store(0); g_fw.store(0); g_fh.store(0);
  g_boot_error.clear();
  retro_set_environment(nap_env);
  retro_set_video_refresh(nap_video);
  retro_set_audio_sample(nap_audio_sample);
  retro_set_audio_sample_batch(nap_audio_batch);
  retro_set_input_poll(nap_input_poll);
  retro_set_input_state(nap_input_state);
  retro_init();
  retro_game_info gi; memset(&gi,0,sizeof(gi));
  gi.path = gamePath.c_str();
  bool ok = retro_load_game(&gi);
  if (!ok) {
    g_boot_error = "retro_load_game FAILED path=" + gamePath;
    retro_deinit();
    snprintf(out,sizeof(out),"PS1_BOOT_FAIL %s (zkontroluj BIOS v systemove slozce a cestu ke hre)", g_boot_error.c_str());
    NAPLOG("%s", out);
    return env->NewStringUTF(out);
  }
  g_loaded.store(true);
  retro_system_av_info av; memset(&av,0,sizeof(av));
  retro_get_system_av_info(&av);
  g_fps = av.timing.fps > 1 ? av.timing.fps : 60.0;
  const int gen = g_generation.load();
  g_running.store(true);
  g_worker = std::thread(nap_worker, gen);
  snprintf(out,sizeof(out),"PS1_BOOT_OK path=%s fps=%.2f baseRes=%ux%u sampleRate=%.0f sysdir=%s",
    gamePath.c_str(), av.timing.fps, av.geometry.base_width, av.geometry.base_height, av.timing.sample_rate, g_sysdir.c_str());
  NAPLOG("%s", out);
  return env->NewStringUTF(out);
}
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1Status(JNIEnv *env, jclass) {
  char out[512];
  snprintf(out,sizeof(out),"PS1_RUN running=%s frames=%llu dupes=%llu res=%dx%d pixfmt=%d audioFifoFrames=%zu audioDropped=%llu fps=%.2f err=%s",
    g_running.load()?"YES":"NO",(unsigned long long)g_frames.load(),(unsigned long long)g_dupe_frames.load(),
    g_fw.load(),g_fh.load(),g_pixfmt.load(),({std::lock_guard<std::mutex> al(g_amutex); g_afifo.size()/2;}),(unsigned long long)g_audio_samples_dropped.load(),g_fps,
    g_boot_error.empty()?"none":g_boot_error.c_str());
  return env->NewStringUTF(out);
}
// BUILD2SA2B: vyzvednuti snimku pro nahled (a pozdeji TextureView). Vraci (w<<16)|h, 0 = nic.
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1GrabFrame(JNIEnv *env, jclass, jintArray out) {
  std::lock_guard<std::mutex> lock(g_frame_mutex);
  const int w = g_fw.load(), h = g_fh.load();
  if (w <= 0 || h <= 0 || g_frame_argb.size() < (size_t)w * h || !out) return 0;
  jsize cap = env->GetArrayLength(out);
  if (cap < w * h) return -(w << 16 | h); // buffer maly - Java si ho zvetsi
  env->SetIntArrayRegion(out, 0, w * h, (const jint*)g_frame_argb.data());
  return (w << 16) | h;
}
// BUILD2SA3: Java audio vlakno si tahne stereo framy; vraci pocet framu.
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1PullAudio(JNIEnv *env, jclass, jshortArray out, jint frames) {
  if (!out || frames <= 0) return 0;
  std::lock_guard<std::mutex> lock(g_amutex);
  size_t have = g_afifo.size() / 2;
  size_t n = have < (size_t)frames ? have : (size_t)frames;
  if (!n) return 0;
  env->SetShortArrayRegion(out, 0, (jsize)(n * 2), (const jshort*)g_afifo.data());
  g_afifo.erase(g_afifo.begin(), g_afifo.begin() + n * 2);
  return (jint)n;
}
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1Stop(JNIEnv *env, jclass) {
  std::lock_guard<std::mutex> life(g_life_mutex);
  g_generation.fetch_add(1); g_running.store(false);
  if (g_worker.joinable()) g_worker.join();
  if (g_loaded.exchange(false)) { retro_unload_game(); retro_deinit(); }
  NAPLOG("BUILD2SA2 PS1 stop frames=%llu", (unsigned long long)g_frames.load());
  return env->NewStringUTF("PS1_STOPPED");
}
