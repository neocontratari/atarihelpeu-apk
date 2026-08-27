// egl_core_demo_glue.c
// ============================================================
//  KROK B: propojeni rendereru s DEMO verzi (barevne pruhy).
//  Renderer vola core_init / core_step / core_get_frame.
//  V plne verzi je napojuje core_ps1.c (napojeni na PS1 jadro),
//  ktery jsme do kroku B ZAMERNE nedali (at netaha druhe PS1
//  jadro). Tady je tedy nahradime demem - presne to, co v kroku
//  B chceme videt. V kroku C to vymenime za skutecne PS1.
// ============================================================

#include "egl_core_api.h"

void core_init(void* java_vm, const char* internal_data_path) {
    (void)java_vm;
    (void)internal_data_path;
    // demo nic inicializovat nepotrebuje
}

void core_step(void) {
    demo_step();
}

bool core_get_frame(CoreFrame* out) {
    return demo_get_frame(out);
}
