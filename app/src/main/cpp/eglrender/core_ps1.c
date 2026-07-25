// core_ps1.c - napojeni SKUTECNEHO PS1 jadra (libnapps1core.so z emu10)
// pres standardni libretro rozhrani, ktere jadro samo exportuje (retro_*).
// Zadne hadani podpisu: libretro je verejne zdokumentovany protokol.
//
// Chovani je neprustrelne: kdyz cokoliv selze (chybi povoleni uloziste,
// neni hra, jadro nejde nacist), bezi dal zalozni demo vzor a v logu
// na 8765/log je presne receno proc.
//
// V teto verzi: pouze OBRAZ. Zvuk a ovladani prijdou jako dalsi kroky.

#define _POSIX_C_SOURCE 200809L

#include "egl_core_api.h"
#include "egl_logserver.h"

#include <android/log.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define TAG "EGLRender"
#define P1LOG(...) do { __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__); ls_log(__VA_ARGS__); } while (0)

// ------------------------------------------------------------------
// Minimalni vytazek z libretro.h (verejny standard, stabilni ABI)
// ------------------------------------------------------------------
enum {
    RETRO_ENVIRONMENT_GET_CAN_DUPE         = 3,
    RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY = 9,
    RETRO_ENVIRONMENT_SET_PIXEL_FORMAT     = 10,
    RETRO_ENVIRONMENT_GET_LOG_INTERFACE    = 27,
    RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY   = 31,
};
enum { RETRO_PIXEL_FORMAT_RGB565 = 2 };

struct retro_game_info { const char* path; const void* data; size_t size; const char* meta; };
struct retro_system_info {
    const char* library_name;
    const char* library_version;
    const char* valid_extensions;
    bool need_fullpath;
    bool block_extract;
};
struct retro_game_geometry { unsigned base_width, base_height, max_width, max_height; float aspect_ratio; };
struct retro_system_timing { double fps, sample_rate; };
struct retro_system_av_info { struct retro_game_geometry geometry; struct retro_system_timing timing; };
typedef void (*retro_log_printf_t)(int level, const char* fmt, ...);
struct retro_log_callback { retro_log_printf_t log; };

typedef bool    (*env_t)(unsigned cmd, void* data);
typedef void    (*video_t)(const void* data, unsigned w, unsigned h, size_t pitch);
typedef void    (*audio_t)(int16_t l, int16_t r);
typedef size_t  (*audio_batch_t)(const int16_t* data, size_t frames);
typedef void    (*input_poll_t)(void);
typedef int16_t (*input_state_t)(unsigned port, unsigned device, unsigned index, unsigned id);

// ------------------------------------------------------------------
// Nactene funkce jadra
// ------------------------------------------------------------------
static void     (*p_set_environment)(env_t);
static void     (*p_set_video)(video_t);
static void     (*p_set_audio)(audio_t);
static void     (*p_set_audio_batch)(audio_batch_t);
static void     (*p_set_input_poll)(input_poll_t);
static void     (*p_set_input_state)(input_state_t);
static void     (*p_init)(void);
static bool     (*p_load_game)(const struct retro_game_info*);
static void     (*p_run)(void);
static void     (*p_get_system_info)(struct retro_system_info*);
static void     (*p_get_av_info)(struct retro_system_av_info*);
static unsigned (*p_api_version)(void);

static bool s_ps1_running = false;   // jadro bezi a dodava obraz
static char s_ps1_dir[300]    = "";  // <interni slozka>/ps1 (hry z wifi stranky)
static char s_system_dir[340] = "";  // <interni slozka>/ps1/bios
static char s_save_dir[256]   = "/data/local/tmp";
static char s_game[512];

// posledni snimek od jadra (jadro vlastni pamet, my si ji jen pujcujeme)
static const void* s_vid_data  = NULL;
static unsigned    s_vid_w     = 0;
static unsigned    s_vid_h     = 0;
static size_t      s_vid_pitch = 0;

// ------------------------------------------------------------------
// Callbacky pro jadro
// ------------------------------------------------------------------
static void core_log_cb(int level, const char* fmt, ...) {
    (void)level;
    char buf[200];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    size_t n = strlen(buf);
    while (n && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[--n] = 0;
    if (n) P1LOG("PS1c: %s", buf);
}

static bool env_cb(unsigned cmd, void* data) {
    switch (cmd) {
        case RETRO_ENVIRONMENT_GET_CAN_DUPE:
            *(bool*)data = true;
            return true;
        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
            *(const char**)data = s_system_dir;
            return true;
        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
            *(const char**)data = s_save_dir;
            return true;
        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
            // Domluva s jadrem: chceme RGB565 - presne to umi nase textura.
            return (*(const int*)data == RETRO_PIXEL_FORMAT_RGB565);
        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
            struct retro_log_callback* lc = (struct retro_log_callback*)data;
            lc->log = core_log_cb;
            return true;
        }
        default:
            return false;  // vse ostatni: jadro pouzije sve vychozi hodnoty
    }
}

static void video_cb(const void* data, unsigned w, unsigned h, size_t pitch) {
    if (data) {
        s_vid_data  = data;
        s_vid_w     = w;
        s_vid_h     = h;
        s_vid_pitch = pitch;
    }
    // data == NULL znamena "stejny snimek jako minule" - nechame posledni
}

// ==================================================================
//  ZVUK — v ceste A resi zvuk JADRO (nap_ps1_native pres OpenSL ES),
//  protoze jadro vola svoje vlastni audio callbacky (nap_audio_batch),
//  nastavene v nap_ps1_egl_boot_c. Tyhle core_ps1 callbacky se v ceste
//  A NEPOUZIJI - jsou tu jen pro pripad softwarove zalozni cesty, a to
//  bez vlastniho vystupu (jen spocitaji, kolik zvuku by teklo).
// ==================================================================
static void   audio_cb(int16_t l, int16_t r) { (void)l; (void)r; }
static size_t audio_batch_cb(const int16_t* d, size_t frames) {
    (void)d;
    // MUJ LOG: kdyby se tahle (softwarova zalozni) cesta pouzila.
    static long dbg_aud_frames = 0, dbg_aud_calls = 0;
    dbg_aud_calls++;
    dbg_aud_frames += (long)frames;
    if (dbg_aud_calls % 300 == 0)
        P1LOG("MUJLOG zvuk(zaloha): %ld davek, %ld vzorku", dbg_aud_calls, dbg_aud_frames);
    return frames;
}

// ==================================================================
//  OVLADANI — v C. NativeActivity chyta dotyk (AInputEvent), prevede
//  na stav PS1 tlacitek do g_pad_state, a input_state_cb ho vraci
//  jadru. Doted vracel nulu ("ovladani = dalsi krok") a rozlozeni
//  tlacitek delala Java pres WebView overlay. Ted je to tady.
// ==================================================================
#include <stdatomic.h>
static atomic_uint g_pad_state = 0; // bitmapa RETRO_DEVICE_ID_JOYPAD_*

// egl_main.c (obsluha dotyku) sem strka stav tlacitek.
void core_set_pad(unsigned state) { atomic_store(&g_pad_state, state); }

// CESTA A: ukazatele na dvirka jadra (gpu-gles textura bez procesoru).
static unsigned (*p_egl_grab)(int*,int*,int*,int*) = NULL;
static int      (*p_egl_vram_w)(void) = NULL;
static int      (*p_egl_vram_h)(void) = NULL;
static int      s_use_texture = 0; // 1 = gpu-gles cesta bezi
static int      s_booted_egl = 0;  // 1 = boot probehl pres cestu A
static void   (*s_egl_tick)(void) = NULL; // tick funkce jadra (retro_run+sync)

bool     core_use_texture(void) { return s_use_texture != 0; }
unsigned core_get_texture(int* x, int* y, int* w, int* h) {
    if (!s_use_texture || !p_egl_grab) return 0;
    return p_egl_grab(x, y, w, h);
}
int core_vram_w(void) { return p_egl_vram_w ? p_egl_vram_w() : 1024; }
int core_vram_h(void) { return p_egl_vram_h ? p_egl_vram_h() : 512; }

static void   input_poll_cb(void) {}
static int16_t input_state_cb(unsigned port, unsigned device, unsigned index, unsigned id) {
    (void)index;
    if (port != 0) return 0;
    if (device != 1 /*RETRO_DEVICE_JOYPAD*/) return 0;
    unsigned st = atomic_load(&g_pad_state);
    return (st >> id) & 1u;
}

// ------------------------------------------------------------------
// Hledani hry v Download/AtariHelp/PS1 (stejne misto, kam ji uklada emu10)
// ------------------------------------------------------------------
static bool ends_with_nocase(const char* s, const char* suf) {
    size_t ls = strlen(s), lf = strlen(suf);
    if (lf > ls) return false;
    return strcasecmp(s + ls - lf, suf) == 0;
}

static bool pick_in_dir(const char* dir, const char* ext, char* out, size_t outsz) {
    DIR* d = opendir(dir);
    if (!d) return false;
    struct dirent* e;
    bool found = false;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        if (ends_with_nocase(e->d_name, ext)) {
            snprintf(out, outsz, "%s/%s", dir, e->d_name);
            found = true;
            break;
        }
    }
    closedir(d);
    return found;
}

static bool find_game(char* out, size_t outsz) {
    static const char* EXTS[] = { ".cue", ".chd", ".m3u", ".bin" };

    // 1) HLAVNI CESTA: hry nahrane pres wifi stranku http://IP:8765/
    //    (interni slozka aplikace - zadne opravneni neni potreba)
    for (size_t i = 0; i < sizeof EXTS / sizeof *EXTS; i++) {
        if (pick_in_dir(s_ps1_dir, EXTS[i], out, outsz)) return true;
    }
    P1LOG("PS1: ve slozce aplikace zatim zadna hra. Na PC otevri http://IP-TELEFONU:8765/ a nahraj .cue + .bin (nebo .chd).");

    // 2) ZALOZNI CESTA: stara slozka emu10 v Download
    //    (na novejsim Androidu ji system aplikacim nedava - pak tise selze)
    static const char* ROOT = "/storage/emulated/0/Download/AtariHelp/PS1";
    DIR* d = opendir(ROOT);
    if (!d) return false;

    for (size_t i = 0; i < sizeof EXTS / sizeof *EXTS; i++) {
        if (pick_in_dir(ROOT, EXTS[i], out, outsz)) { closedir(d); return true; }
    }
    struct dirent* e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        char sub[400];
        snprintf(sub, sizeof sub, "%s/%s", ROOT, e->d_name);
        DIR* t = opendir(sub);       // jen skutecne slozky pujdou otevrit
        if (!t) continue;
        closedir(t);
        for (size_t i = 0; i < sizeof EXTS / sizeof *EXTS; i++) {
            if (pick_in_dir(sub, EXTS[i], out, outsz)) { closedir(d); return true; }
        }
    }
    closedir(d);
    return false;
}

// ------------------------------------------------------------------
// Zasuvka core_api - verejne funkce
// ------------------------------------------------------------------
void core_init(void* java_vm, const char* internal_data_path) {
    (void)java_vm;  // libretro cesta zadnou Javu nepotrebuje

    if (internal_data_path && *internal_data_path) {
        snprintf(s_save_dir, sizeof s_save_dir, "%s", internal_data_path);
    }
    snprintf(s_ps1_dir, sizeof s_ps1_dir, "%s/ps1", s_save_dir);
    mkdir(s_ps1_dir, 0700);
    snprintf(s_system_dir, sizeof s_system_dir, "%s/bios", s_ps1_dir);
    mkdir(s_system_dir, 0700);

    P1LOG("PS1: nacitam jadro libnapps1core.so ...");
    void* h = dlopen("libnapps1core.so", RTLD_NOW | RTLD_LOCAL);
    if (!h) {
        P1LOG("PS1: dlopen selhal (%s) -> bezi demo vzor", dlerror());
        return;
    }

    *(void**)&p_set_environment = dlsym(h, "retro_set_environment");
    *(void**)&p_set_video       = dlsym(h, "retro_set_video_refresh");
    *(void**)&p_set_audio       = dlsym(h, "retro_set_audio_sample");
    *(void**)&p_set_audio_batch = dlsym(h, "retro_set_audio_sample_batch");
    *(void**)&p_set_input_poll  = dlsym(h, "retro_set_input_poll");
    *(void**)&p_set_input_state = dlsym(h, "retro_set_input_state");
    *(void**)&p_init            = dlsym(h, "retro_init");
    *(void**)&p_load_game       = dlsym(h, "retro_load_game");
    *(void**)&p_run             = dlsym(h, "retro_run");
    *(void**)&p_get_system_info = dlsym(h, "retro_get_system_info");
    *(void**)&p_get_av_info     = dlsym(h, "retro_get_system_av_info");
    *(void**)&p_api_version     = dlsym(h, "retro_api_version");

    // CESTA A: dvirka pro gpu-gles obraz bez procesoru.
    static int (*p_egl_boot)(const char*,const char*) = NULL;
    static void (*p_egl_tick)(void) = NULL;
    *(void**)&p_egl_boot   = dlsym(h, "nap_ps1_egl_boot_c");
    *(void**)&p_egl_tick   = dlsym(h, "nap_ps1_egl_tick_c");
    *(void**)&p_egl_grab   = dlsym(h, "nap_ps1_egl_grab");
    *(void**)&p_egl_vram_w = dlsym(h, "nap_ps1_egl_vram_w");
    *(void**)&p_egl_vram_h = dlsym(h, "nap_ps1_egl_vram_h");

    if (p_egl_boot && p_egl_tick && p_egl_grab) {
        // Jadro umi cestu A - gpu-gles obraz. Boot jde pres nej (udela
        // gpu-gles init + retro_init + load_game na TOMHLE vlakne).
        P1LOG("PS1: jadro umi gpu-gles cestu A - ostry obraz bez procesoru");
        // audio a vstup callbacky nastavi boot uvnitr jadra; ale nase
        // audio_batch_cb a input_state_cb jsou v nap_ps1_native (nap_audio_*),
        // ne tady - CESTA A pouziva jadro vlastni callbacky (nap_audio_*),
        // v tomhle rezimu nebezi (zvuk resi jadro pres svuj audio batch).
        // Proto zvuk necháme na ceste A: viz nap_ps1_native audio.
        // ktere posilaji zvuk do OpenSL ES v jadre. Zvuk tedy resi jadro.
        char sysdir[340];
        snprintf(sysdir, sizeof sysdir, "%s", s_system_dir);
        if (!find_game(s_game, sizeof s_game)) {
            P1LOG("PS1: bez hry -> bezi demo vzor");
            return;
        }
        int r = p_egl_boot(sysdir, s_game);
        if (r >= 0) {
            s_use_texture = (r == 1) ? 1 : 0;
            s_egl_tick = p_egl_tick;
            s_booted_egl = 1;
            P1LOG("PS1: CESTA A boot OK (textura=%s)", s_use_texture ? "ANO ostry gpu-gles" : "ne - jen software");
            return;
        }
        P1LOG("PS1: CESTA A boot selhal (%d) -> zkousim softwarovou cestu", r);
        // propad do stare softwarove cesty nize
    }

    if (!p_set_environment || !p_set_video || !p_set_audio || !p_set_audio_batch ||
        !p_set_input_poll || !p_set_input_state || !p_init || !p_load_game ||
        !p_run || !p_get_system_info || !p_get_av_info) {
        P1LOG("PS1: jadru chybi cast libretro funkci -> bezi demo vzor");
        return;
    }

    struct retro_system_info si;
    memset(&si, 0, sizeof si);
    p_get_system_info(&si);
    P1LOG("PS1: jadro se hlasi: %s %s (api v%u)",
          si.library_name ? si.library_name : "?",
          si.library_version ? si.library_version : "?",
          p_api_version ? p_api_version() : 0);

    p_set_environment(env_cb);
    p_set_video(video_cb);
    p_set_audio(audio_cb);
    p_set_audio_batch(audio_batch_cb);
    p_set_input_poll(input_poll_cb);
    p_set_input_state(input_state_cb);
    p_init();
    P1LOG("PS1: retro_init OK. BIOS slozka: %s (kdyz je prazdna, jede vestaveny nahradni BIOS)",
          s_system_dir);

    if (!find_game(s_game, sizeof s_game)) {
        P1LOG("PS1: bez hry -> bezi demo vzor");
        return;
    }
    P1LOG("PS1: bootuji hru: %s", s_game);

    struct retro_game_info gi;
    memset(&gi, 0, sizeof gi);
    gi.path = s_game;   // jadro chce plnou cestu (need_fullpath)
    if (!p_load_game(&gi)) {
        P1LOG("PS1: retro_load_game selhal -> bezi demo vzor");
        return;
    }

    struct retro_system_av_info av;
    memset(&av, 0, sizeof av);
    p_get_av_info(&av);
    // Softwarova zaloha nema vlastni zvukovy vystup (zvuk resi cesta A
    // pres jadro). Tady jen log, ze hra nabehla.
    P1LOG("PS1: hra nabootovala (softwarova zaloha). Zaklad %ux%u, %.2f snimku/s",
          av.geometry.base_width, av.geometry.base_height, av.timing.fps);

    s_ps1_running = true;
}

void core_step(void) {
    if (s_booted_egl) {
        if (s_egl_tick) s_egl_tick(); // CESTA A: retro_run + gpu-gles sync v jadre
    } else if (s_ps1_running) {
        p_run();          // jeden snimek emulace (softwarova cesta)
    } else {
        demo_step();
    }
}

bool core_get_frame(CoreFrame* out) {
    if (s_ps1_running) {
        if (!s_vid_data) return false;   // jadro jeste neposlalo prvni obraz
        out->pixels = s_vid_data;
        out->width  = (int)s_vid_w;
        out->height = (int)s_vid_h;
        out->pitch  = (int)s_vid_pitch;
        out->format = CORE_FMT_RGB565;   // vyjednano pres SET_PIXEL_FORMAT
        return true;
    }
    return demo_get_frame(out);
}
