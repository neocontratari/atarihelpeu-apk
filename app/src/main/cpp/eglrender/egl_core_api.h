#pragma once
#include <stdbool.h>

// ==================================================================
//  ZASUVKA PRO JADRO EMULATORU (Sega / PS1)
//
//  Renderer se kazdy snimek zepta jadra na aktualni obraz. Jadro
//  vraci UKAZATEL na svuj framebuffer - zadne kopirovani na CPU.
//  Rozliseni se smi KDYKOLI zmenit (PS1 prepina napr. 256x240,
//  512x240, 640x480 podle vnitrniho stavu hry) - renderer si texturu
//  automaticky prestavi a letterbox prepocita.
//
//  Napojeni skutecneho jadra = nahradit core_demo.c souborem, ktery
//  tyhle dve funkce namapuje na jadro:
//   - PS1: pixels = buffer, ktery plni gpulib (fmt=1 v tvych lozich
//     -> CORE_FMT_XRGB8888), width/height = dispW/dispH.
//   - Sega: pixels = vystup renderu mostu, 320x240 ci 256x224,
//     format podle jadra (RGB565 nebo RGBA8888).
// ==================================================================

typedef enum {
    CORE_FMT_RGBA8888 = 0,  // bajty R,G,B,A
    CORE_FMT_XRGB8888 = 1,  // format PS1 jadra (bajty B,G,R,X)
    CORE_FMT_RGB565   = 2,  // 16bitova varianta
} CoreFormat;

typedef struct {
    const void* pixels;  // framebuffer jadra
    int         width;   // aktualni sirka hry
    int         height;  // aktualni vyska hry
    int         pitch;   // bajtu na radek (0 = radky tesne za sebou)
    CoreFormat  format;
} CoreFrame;

// Jednorazova priprava jadra pri startu aplikace.
// java_vm a interni slozka aplikace (pro pametove karty / save stavy).
void core_init(void* java_vm, const char* internal_data_path);

// Krok jadra o jeden snimek (vola se 60x za vterinu).
void core_step(void);

// Vrati aktualni snimek jadra. false = jadro jeste nema obraz
// (renderer nakresli jen pozadi a zkusi to dalsi snimek).
bool core_get_frame(CoreFrame* out);

// ==================================================================
//  CESTA A — gpu-gles textura primo (ostry obraz, bez procesoru).
//  Kdyz core_use_texture() vrati true, renderer NEbere pixely
//  (core_get_frame), ale hotovou GL texturu z gpu-gles: id + vyrez
//  ve VRAM texture. Kresli ji rovnou. Zadny prenos do procesoru.
// ==================================================================
bool     core_use_texture(void);   // true = jede gpu-gles cesta A
unsigned core_get_texture(int* x, int* y, int* w, int* h); // 0 = zatim nic
int      core_vram_w(void);
int      core_vram_h(void);

// ------------------------------------------------------------------
// Interni: zalozni demo vzor (bezi, dokud nenabehne skutecne jadro)
// ------------------------------------------------------------------
void demo_step(void);
bool demo_get_frame(CoreFrame* out);
