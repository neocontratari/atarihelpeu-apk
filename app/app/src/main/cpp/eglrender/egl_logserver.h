#pragma once

// Mini HTTP log server - stejny princip jako 8765/log ve tvem emu10.
// V prohlizeci na PC (stejna wifi): http://IP-TELEFONU:8765/log

// Nastavi interni slozku aplikace - vytvori <slozka>/ps1 a /ps1/bios,
// kam stranka http://IP:8765/ uklada nahrane hry a BIOS.
// Volat PRED logserver_start().
void logserver_set_upload_dir(const char* internal_dir);

// Spusti server na danem portu (bezi ve vlastnim vlakne).
// Kdyz port nejde otevrit (napr. ho drzi bezici emu10), jen to zaloguje
// do logcatu a render jede dal - server neni pro beh nutny.
void logserver_start(int port);

// Ulozi radek do kruhove pameti (posledních ~400 radku), kterou server vypisuje.
void ls_log(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
