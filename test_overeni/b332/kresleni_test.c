/* B332: maly program pro PlayStation (PS-EXE), ktery KRESLI jako hra - test kresleni ve vlakne.
 *
 * Kazdy snimek postavi seznam prikazu GPU v pameti (jako hry: "ordering table" = retez paketu)
 * a posle ho DMA kanalem 2 do GPU: nastaveni kresleni do zadniho bufferu (dvojity buffer:
 * y = 0 a y = 256), vyplneni pozadi, 160 trojuhelniku s prechodem barev, 40 poloprusvitnych
 * ctyruhelniku, 24 ctyruhelniku s texturou (textura 64x64 nahrana na zacatku do VRAM x = 640),
 * 30 car, kopie bloku ve VRAM a nakonec znacku 16x8 bodu vlevo nahore s cislem snimku
 * (cervena = cislo & 31, zelena = (cislo >> 5) & 31) - test podle ni pozna, ktery snimek vidi.
 * Kazdy 8. snimek precte 8x4 bodu z VRAM zpet do procesoru (jako hry: GPUREAD - kresleni
 * ve vlakne se tu musi dokreslit, nez se cte) a secte je. Po VBlanku prepne zobrazeni na
 * prave dokresleny buffer.
 * Vysledky v RAM pro test (0x80180000): [0] snimku hotovo, [1] soucet ctenych bodu,
 * [2] pocet cteni, [3] znacka 0xB332B332 (program bezi).
 *
 * Preklad: clang (mipsel, -march=mips1 = R3000, load delay hlida prekladac), ld.lld s vlastnim
 * rozvrzenim (kresleni_test.ld), llvm-objcopy -> holy kod, hlavicka PS-EXE (udelej_kresleni_exe.py).
 * Bez knihovny C, bez BIOSu (preruseni vypnuta, VBlank se cte z I_STAT).
 */
#define REG(a) (*(volatile unsigned *)(a))
#define GP0 REG(0x1f801810u)
#define GPUREAD REG(0x1f801810u)
#define GP1 REG(0x1f801814u)
#define GPUSTAT REG(0x1f801814u)
#define I_STAT REG(0x1f801070u)
#define I_MASK REG(0x1f801074u)
#define DPCR REG(0x1f8010f0u)
#define D2_MADR REG(0x1f8010a0u)
#define D2_BCR REG(0x1f8010a4u)
#define D2_CHCR REG(0x1f8010a8u)

#define VYSL ((volatile unsigned *)0x80180000u)
#define PAKETY ((unsigned *)0x80100000u)

__asm__(".section .text.start,\"ax\",@progbits\n"
        ".set noreorder\n"
        ".globl _start\n"
        "_start:\n"
        "  lui $sp, 0x801f\n"
        "  ori $sp, $sp, 0xfff0\n"
        "  mtc0 $zero, $12\n"       /* preruseni vypnuta (VBlank se cte z I_STAT) */
        "  nop\n"
        "  jal hlavni\n"
        "  nop\n"
        "1: j 1b\n"
        "  nop\n"
        ".set reorder\n"
        ".text\n");

static unsigned xs(unsigned *s)
{
    unsigned x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *s = x;
}

static void gp0(unsigned v)
{
    while (!(GPUSTAT & (1u << 26)))   /* GPU pripraveno na prikaz */
        ;
    GP0 = v;
}

static unsigned xy(int x, int y) { return ((unsigned)(y & 0xffff) << 16) | (unsigned)(x & 0xffff); }

/* paket v retezu: hlavicka (pocet slov << 24 | adresa dalsiho), za ni prikazy */
static unsigned *paket(unsigned *p, int slov)
{
    unsigned *dalsi = p + 1 + slov;
    p[0] = ((unsigned)slov << 24) | ((unsigned)dalsi & 0xffffffu);
    return p + 1;
}

static void cekej_vblank(void)
{
    I_STAT = ~1u;                     /* smazat stary priznak */
    while (!(I_STAT & 1u))
        ;
}

static unsigned cti_zpet(int x, int y)   /* 8x4 bodu z VRAM do procesoru (GPUREAD) */
{
    unsigned s = 0;
    int i;
    gp0(0xc0000000u);
    gp0(xy(x, y));
    gp0(xy(8, 4));
    while (!(GPUSTAT & (1u << 27)))   /* GPU pripraveno poslat VRAM */
        ;
    for (i = 0; i < 8 * 4 / 2; i++)
        s += GPUREAD;
    return s;
}

void hlavni(void)
{
    unsigned sem = 0x12345678u, n, i;

    VYSL[0] = 0; VYSL[1] = 0; VYSL[2] = 0; VYSL[3] = 0xb332b332u;
    I_MASK = 0;
    GP1 = 0x00000000u;                /* reset GPU */
    GP1 = 0x08000001u;                /* 320x240, NTSC, 15 bit, bez prokladani */
    GP1 = 0x06c60260u;                /* vodorovny rozsah */
    GP1 = 0x07040010u;                /* svisly rozsah 16..256 */
    GP1 = 0x05000000u;                /* zobrazovat od 0,0 */
    GP1 = 0x03000000u;                /* zobrazeni zapnuto */
    DPCR |= 0x800u;                   /* DMA kanal 2 (GPU) */

    /* textura 64x64 (15 bit) do VRAM x = 640, y = 0 */
    gp0(0xa0000000u);
    gp0(xy(640, 0));
    gp0(xy(64, 64));
    for (i = 0; i < 64 * 64; i += 2) {
        unsigned x = i & 63, y = i >> 6, a, b;
        a = (((x ^ y) & 8) ? 0x7c1fu : 0x03e0u) ^ (x << 5) ^ y;
        b = (((x + 1) ^ y) & 8) ? 0x001fu : 0x7fe0u;
        gp0((a & 0x7fffu) | ((b ^ (y << 10)) & 0x7fffu) << 16);
    }

    for (n = 0;; n++) {
        const int by = (n & 1) ? 256 : 0;
        unsigned *p = PAKETY, *c;
        unsigned s = sem ^ 0x9e3779b9u;   /* stejne tvary kazdy snimek, jen se posouvaji */

        c = paket(p, 4); p = c + 4;
        c[0] = 0xe1000600u;               /* dithering, kreslit i do zobrazovane oblasti */
        c[1] = 0xe3000000u | ((unsigned)by << 10);
        c[2] = 0xe4000000u | ((unsigned)(by + 239) << 10) | 319u;
        c[3] = 0xe5000000u | ((unsigned)by << 11);

        c = paket(p, 3); p = c + 3;       /* pozadi */
        c[0] = 0x02600020u | (((n << 1) & 0xffu) << 8);
        c[1] = xy(0, by);
        c[2] = xy(320, 240);

        for (i = 0; i < 160; i++) {       /* trojuhelniky s prechodem barev */
            unsigned r = xs(&s), q = xs(&s);
            int x0 = (int)((r & 0xff) + ((r >> 8) & 0x3f)), y0 = (int)(((r >> 16) & 0x7f) + ((r >> 24) & 0x6f));
            int dx = (int)(n & 63) - 32;
            c = paket(p, 6); p = c + 6;
            c[0] = 0x30000000u | (q & 0xffffffu);
            c[1] = xy(x0 + dx, y0);
            c[2] = (q >> 8) & 0xffffffu;
            c[3] = xy(x0 + 40 + (int)(q & 31), y0 + 10 + (int)((q >> 5) & 15));
            c[4] = r & 0xffffffu;
            c[5] = xy(x0 - 10 + (int)((q >> 10) & 15), y0 + 50 + (int)((q >> 14) & 31) - dx);
        }
        for (i = 0; i < 40; i++) {        /* poloprusvitne ctyruhelniky */
            unsigned r = xs(&s);
            int x0 = (int)((r & 0xff) + ((r >> 8) & 0x3f)), y0 = (int)(((r >> 16) & 0x7f) + ((r >> 24) & 0x6f));
            int w = 20 + (int)((r >> 3) & 31), h = 16 + (int)((r >> 11) & 31);
            x0 += (int)((n * 3) & 31);
            c = paket(p, 5); p = c + 5;
            c[0] = 0x2a000000u | (xs(&s) & 0xffffffu);
            c[1] = xy(x0, y0); c[2] = xy(x0 + w, y0); c[3] = xy(x0, y0 + h); c[4] = xy(x0 + w, y0 + h);
        }
        for (i = 0; i < 24; i++) {        /* ctyruhelniky s texturou (15 bit, stranka x = 640) */
            unsigned r = xs(&s);
            int x0 = (int)((r & 0xff) + ((r >> 8) & 0x1f)), y0 = (int)(((r >> 16) & 0x7f) + ((r >> 24) & 0x3f));
            int w = 24 + (int)((r >> 4) & 31);
            y0 += (int)(n & 15);
            c = paket(p, 9); p = c + 9;
            c[0] = 0x2c808080u;
            c[1] = xy(x0, y0);         c[2] = 0x00000000u;
            c[3] = xy(x0 + w, y0);     c[4] = (0x10au << 16) | 63u;
            c[5] = xy(x0, y0 + w);     c[6] = 63u << 8;
            c[7] = xy(x0 + w, y0 + w); c[8] = (63u << 8) | 63u;
        }
        for (i = 0; i < 30; i++) {        /* cary */
            unsigned r = xs(&s);
            c = paket(p, 3); p = c + 3;
            c[0] = 0x40000000u | (xs(&s) & 0xffffffu);
            c[1] = xy((int)(r & 0xff), (int)((r >> 8) & 0xdf));
            c[2] = xy((int)((r >> 16) & 0xff) + (int)(n & 31), (int)((r >> 24) & 0xdf));
        }
        c = paket(p, 4); p = c + 4;       /* kopie bloku ve VRAM: kus textury do obrazu */
        c[0] = 0x80000000u;
        c[1] = xy(640, 0);
        c[2] = xy(240, by + 160);
        c[3] = xy(32, 32);
        c = paket(p, 3); p = c + 3;       /* znacka cisla snimku vlevo nahore (16 x 8) */
        c[0] = 0x02800000u | (((n >> 5) & 31u) << 11) | ((n & 31u) << 3);
        c[1] = xy(0, by);
        c[2] = xy(16, 8);
        p[0] = 0x00ffffffu;               /* konec retezu */

        while (D2_CHCR & (1u << 24))
            ;
        GP1 = 0x04000002u;                /* DMA: procesor -> GP0 */
        D2_MADR = (unsigned)PAKETY & 0xffffffu;
        D2_BCR = 0;
        D2_CHCR = 0x01000401u;            /* retez paketu (linked list) */
        while (D2_CHCR & (1u << 24))
            ;

        if ((n & 7) == 7) {               /* jako hry: cteni z VRAM zpet (musi pockat na dokresleni) */
            VYSL[1] += cti_zpet(100, by + 100);
            VYSL[2] += 1;
        }
        cekej_vblank();
        GP1 = 0x05000000u | ((unsigned)by << 10);   /* zobrazit prave dokresleny buffer */
        VYSL[0] = n + 1;
    }
}
