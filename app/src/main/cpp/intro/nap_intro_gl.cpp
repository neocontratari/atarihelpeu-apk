// nap_intro_gl.cpp
// ====================================================================
//  UVODNI INTRO NATIVNE - stejnou cestou jako PS1.
//
//  PROC TAKHLE A NE VE WEBVIEW:
//  Puvodni intro bezelo jako HTML+JavaScript ve WebView, tedy na
//  HLAVNIM VLAKNE aplikace. TV si od nej musela obraz vyfotit pres
//  PixelCopy CELEHO OKNA - 960 000 bodu plus kopie 3,7 MB, a to vsechno
//  na tom samem vlakne. Vysledek: pri zapnute WEB TV se kousalo vsechno.
//
//  PS1 a Sega to delaji jinak a spravne: kresli na VLASTNI PLOCHU
//  (SurfaceView) z VLASTNIHO VLAKNA pres EGL, a TV dostane obraz PRIMO
//  z jadra - okno se nesnima vubec. Tenhle soubor dela to same.
//
//  Dve plochy, dve vlakna:
//    introSetDisplaySurface(Surface)  -> obraz na telefonu
//    introSetTvSurface(Surface)       -> obraz do enkoderu pro TV
//  Obe kresli tu samou scenu ve stejnem case, takze na TV jde presne
//  to, co na telefonu.
//
//  Plazma i duha se pocitaji ve FRAGMENT SHADERU - na GPU je to zadarmo.
//  Ve WebView jsem je pocital procesorem na malem platne a roztahoval.
// ====================================================================

#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <mutex>
#include "../atari/nap_atari_roms.h"   // ROM od Reneho - pismo je na $E000
#include "nap_intro_zvuk.h"

#define INTRO_LOG(...) __android_log_print(ANDROID_LOG_INFO, "NAP_INTRO", __VA_ARGS__)

namespace {

// ------------------------------------------------------------------
//  Stav
// ------------------------------------------------------------------
std::atomic<bool> g_disp_run{false};
std::atomic<bool> g_tv_run{false};
std::thread g_disp_thread, g_tv_thread;
ANativeWindow *g_disp_win = nullptr, *g_tv_win = nullptr;

// spolecny cas intra - obe plochy kresli tentyz okamzik
std::atomic<long long> g_start_ms{0};
std::atomic<bool> g_hotovo{false};
std::atomic<long long> g_snimku{0};
napintro::Zvuk g_zvuk;
std::atomic<long long> g_vzorku{0};

// POSLEDNI SNIMEK PRO TV.
// Nekreslime do plochy enkoderu vlastnim vlaknem - v projektu je
// zadokumentovane, ze to u PS1 nevyslo (dve vlakna a MediaCodec).
// Funkcni cesta je jina: jadro si odlozi hotovy snimek a JAVA si ho
// pujci (Ps1GlTextureView.borrowFrame / NativeSegaCoreBridge.grabFrame).
// Delame to stejne - jedno vlakno, zadny soubeh, a okno se pritom
// porad nesnima.
std::mutex g_tv_zamek;
std::vector<unsigned char> g_tv_body;      // RGBA
int g_tv_w = 0, g_tv_h = 0;
bool g_tv_novy = false;
std::atomic<bool> g_tv_chce{false};

long long ted_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

// ------------------------------------------------------------------
//  Shadery
// ------------------------------------------------------------------
const char *VS = R"(
attribute vec2 aPos;
varying vec2 vUV;
void main(){
  vUV = aPos * 0.5 + 0.5;
  gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

// Pozadi: plazma + hvezdy + rastrove pruhy, vsechno na GPU.
// uFaze  0=Atari 1=Sega 2=PS1 3=KKT  - kazda cast ma svou barevnost
const char *FS_POZADI = R"(
precision mediump float;
varying vec2 vUV;
uniform float uCas;
uniform vec2  uRozmer;
uniform float uFaze;
uniform float uJas;

float sum(vec2 p){ return fract(sin(dot(p, vec2(12.9898,78.233))) * 43758.5453); }

void main(){
  vec2 uv = vUV;
  vec2 p  = (uv - 0.5) * vec2(uRozmer.x/uRozmer.y, 1.0);

  // --- plazma ---
  float v = sin(p.x*3.1 + uCas*0.7)
          + sin(p.y*2.7 - uCas*0.5)
          + sin((p.x+p.y)*2.2 + uCas*0.9)
          + sin(length(p)*4.0 - uCas*1.3);
  v = v * 0.25 + 0.5;

  // barevnost podle casti intra
  vec3 a = vec3(0.10, 0.55, 0.15);   // Atari - zelena
  vec3 b = vec3(0.15, 0.25, 0.85);   // Sega  - modra
  vec3 c = vec3(0.65, 0.12, 0.55);   // PS1   - fialova
  vec3 d = vec3(0.80, 0.10, 0.18);   // KKT   - cervena
  vec3 zaklad;
  if(uFaze < 1.0)      zaklad = mix(a, b, uFaze);
  else if(uFaze < 2.0) zaklad = mix(b, c, uFaze - 1.0);
  else                 zaklad = mix(c, d, clamp(uFaze - 2.0, 0.0, 1.0));

  vec3 barva = zaklad * (0.18 + 0.62*v*v);

  // --- rastrove pruhy ---
  for(int i=0;i<5;i++){
    float fi = float(i);
    float y  = 0.5 + sin(uCas*0.55 + fi*0.9) * 0.34;
    float d2 = abs(uv.y - y);
    barva += zaklad * (0.55 * exp(-d2*d2*900.0));
  }

  // --- hvezdy ---
  vec2 mrizka = floor(uv * vec2(90.0, 160.0));
  float s = sum(mrizka);
  if(s > 0.9955){
    float bl = 0.55 + 0.45*sin(uCas*2.2 + s*40.0);
    barva += vec3(bl);
  }

  gl_FragColor = vec4(barva * uJas, 1.0);
}
)";

// Kresleni textury (logo, textova obrazovka) s pruhlednosti
const char *VS_TEX = R"(
attribute vec2 aPos;
attribute vec2 aUV;
varying vec2 vUV;
uniform vec4 uRect;      // x,y,w,h v souradnicich -1..1
void main(){
  vUV = aUV;
  gl_Position = vec4(uRect.x + aPos.x*uRect.z, uRect.y + aPos.y*uRect.w, 0.0, 1.0);
}
)";

const char *FS_TEX = R"(
precision mediump float;
varying vec2 vUV;
uniform sampler2D uTex;
uniform float uAlfa;
uniform vec3  uBarva;    // (1,1,1) = beze zmeny
void main(){
  vec4 t = texture2D(uTex, vUV);
  gl_FragColor = vec4(t.rgb * uBarva, t.a * uAlfa);
}
)";

// Duha pro bezici program - na Atari se dela zmenou barvy na kazdem
// radku (POKE 710 v preruseni). Tady je to jeden shader.
// SEGA: perspektivni sachovnicova podlaha, ktera se blizi. Rychla,
// modra, s obzorem - v duchu Mega Drive.
const char *FS_SEGA = R"(
precision mediump float;
varying vec2 vUV;
uniform float uCas;
uniform vec2  uRozmer;
void main(){
  vec2 uv = vUV;
  float obzor = 0.52;
  vec3 barva;
  if(uv.y > obzor){
    barva = mix(vec3(0.02,0.05,0.30), vec3(0.20,0.45,1.00), (uv.y-obzor)/(1.0-obzor));
    float m = smoothstep(0.0,0.35,(uv.y-obzor));
    barva += vec3(0.30,0.55,1.00) * 0.25 * m * (0.5+0.5*sin(uCas*3.0+uv.y*22.0));
  } else {
    float h = (obzor - uv.y);
    float z = 0.06 / max(h, 0.0015);
    float x = (uv.x - 0.5) * z * 3.2;
    float t = z + uCas * 5.5;
    float sach = mod(floor(x) + floor(t), 2.0);
    vec3 a = vec3(0.06,0.10,0.42), b = vec3(0.14,0.30,0.86);
    barva = mix(a, b, sach);
    barva *= clamp(1.4 - h*3.2, 0.15, 1.0);
  }
  gl_FragColor = vec4(barva,1.0);
}
)";

// PS1: otacejici se teleso. Souradnice se pocitaji ve vertex shaderu,
// takze je to skutecne 3D na GPU - ne nakresleny obrazek.
const char *VS_3D = R"(
attribute vec3 aPos;
attribute vec3 aBarva;
uniform float uCas;
uniform float uPomer;
varying vec3 vBarva;
void main(){
  float a = uCas * 0.9, b = uCas * 0.62;
  mat3 ry = mat3(cos(a),0.0,sin(a), 0.0,1.0,0.0, -sin(a),0.0,cos(a));
  mat3 rx = mat3(1.0,0.0,0.0, 0.0,cos(b),-sin(b), 0.0,sin(b),cos(b));
  vec3 p = rx * (ry * aPos);
  p.z += 4.6;                    // dal od kamery, at se teleso vejde
  float f = 2.0 / p.z;
  gl_Position = vec4(p.x*f/uPomer, p.y*f, 0.0, 1.0);
  vBarva = aBarva * (0.42 + 0.58*max(0.0, 1.0 - (p.z-3.6)*0.55));
}
)";
const char *FS_3D = R"(
precision mediump float;
varying vec3 vBarva;
void main(){ gl_FragColor = vec4(vBarva, 1.0); }
)";

// ROZPAD 32 -> 16 -> 8 bitu.
// Scena se nejdriv nakresli do VLASTNI TEXTURY v malem rozliseni
// a pak se roztahne pres obrazovku bez vyhlazeni - odtud kosticky.
// Barvy se navic srazi na mensi pocet odstinu, jako kdyz se jde
// z 32bitove grafiky na 8bitovou.
const char *FS_ROZPAD = R"(
precision mediump float;
varying vec2 vUV;
uniform sampler2D uTex;
uniform float uUrovni;     // kolik odstinu na slozku (256 -> 8)
uniform float uSum;        // kolik do toho pada rusiva
uniform float uCas;
float nah(vec2 p){ return fract(sin(dot(p,vec2(12.9898,78.233)))*43758.5453); }
void main(){
  vec2 uv = vUV;
  // vodorovne trhani obrazu, jako kdyz se rozbiji signal
  float trh = (nah(vec2(floor(uv.y*60.0), floor(uCas*11.0))) - 0.5) * uSum * 0.22;
  uv.x = clamp(uv.x + trh, 0.0, 1.0);
  vec3 c = texture2D(uTex, uv).rgb;
  c = floor(c * uUrovni + 0.5) / uUrovni;     // srazit pocet odstinu
  c += (nah(uv*vec2(311.0,733.0)+uCas) - 0.5) * uSum * 0.30;
  gl_FragColor = vec4(clamp(c,0.0,1.0), 1.0);
}
)";

const char *FS_DUHA = R"(
precision mediump float;
varying vec2 vUV;
uniform float uCas;
vec3 hsv(float h){
  vec3 k = mod(h*6.0 + vec3(5.0,3.0,1.0), 6.0);
  return clamp(min(k, 4.0-k), 0.0, 1.0);
}
void main(){
  float h = fract(vUV.y * 0.85 + uCas * 0.055);
  vec3 c = hsv(h);
  gl_FragColor = vec4(c * 0.62, 1.0);
}
)";

GLuint shader(GLenum typ, const char *zdroj) {
    GLuint s = glCreateShader(typ);
    glShaderSource(s, 1, &zdroj, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[1024]; glGetShaderInfoLog(s, sizeof buf, nullptr, buf);
        INTRO_LOG("shader se neprelozil: %s", buf);
    }
    return s;
}
GLuint program(const char *vs, const char *fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, shader(GL_VERTEX_SHADER, vs));
    glAttachShader(p, shader(GL_FRAGMENT_SHADER, fs));
    glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[1024]; glGetProgramInfoLog(p, sizeof buf, nullptr, buf);
        INTRO_LOG("program se nesestavil: %s", buf);
    }
    return p;
}



// ------------------------------------------------------------------
//  TEXTOVA OBRAZOVKA ATARI
//  40x24 znaku, bunka 8x8 bodu = 320x192. Kresli se do pameti (levne,
//  jen kdyz se text zmeni) a nahraje jako textura - GPU pak kresli
//  jeden obdelnik. Pismo je SKUTECNA znakova sada z ROM, ne napodobenina.
// ------------------------------------------------------------------
const int SL = 40, RA = 24, BUNKA = 8;
const int OBR_W = SL*BUNKA, OBR_H = RA*BUNKA;

struct TextObrazovka {
    char    znaky[RA][SL];
    int     radek = 0, sloupec = 0;
    GLuint  tex = 0;
    bool    zmeneno = true;
    std::vector<unsigned char> body;   // RGBA

    TextObrazovka() { vymaz(); body.resize(OBR_W*OBR_H*4, 0); }

    void vymaz() {
        for (int r=0;r<RA;r++) for (int c=0;c<SL;c++) znaky[r][c]=' ';
        radek=0; sloupec=0; zmeneno=true;
    }
    void pis(char z) {
        if (z=='\n') { radek++; sloupec=0; }
        else {
            if (sloupec<SL) znaky[radek][sloupec]=z;
            sloupec++;
            if (sloupec>=SL) { radek++; sloupec=0; }
        }
        while (radek>=RA) {                     // roluje jako Atari
            for (int r=0;r<RA-1;r++) memcpy(znaky[r], znaky[r+1], SL);
            for (int c=0;c<SL;c++) znaky[RA-1][c]=' ';
            radek--;
        }
        zmeneno=true;
    }
    /** ASCII -> vnitrni kod obrazovky, jak to ma Atari */
    static int vnitrni(char z) {
        unsigned char c=(unsigned char)z;
        if (c>=0x20 && c<=0x5F) return c-0x20;
        if (c>=0x60 && c<=0x7F) return c;
        return 0;
    }
    void nahraj(const unsigned char *font, int r,int g,int b, bool kurzor, bool kurzorVidet) {
        if (!tex) {
            glGenTextures(1,&tex);
            glBindTexture(GL_TEXTURE_2D,tex);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        }
        memset(body.data(), 0, body.size());
        for (int r2=0;r2<RA;r2++) {
            for (int c=0;c<SL;c++) {
                int k = vnitrni(znaky[r2][c]);
                bool plny = (kurzor && kurzorVidet && r2==radek && c==sloupec);
                for (int y=0;y<8;y++) {
                    unsigned char bity = plny ? 0xFF : font[k*8+y];
                    if (!bity) continue;
                    for (int x=0;x<8;x++) {
                        if (!(bity & (0x80>>x))) continue;
                        int px=c*8+x, py=r2*8+y;
                        int o=(py*OBR_W+px)*4;
                        body[o]=(unsigned char)r; body[o+1]=(unsigned char)g;
                        body[o+2]=(unsigned char)b; body[o+3]=255;
                    }
                }
            }
        }
        glBindTexture(GL_TEXTURE_2D,tex);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,OBR_W,OBR_H,0,GL_RGBA,GL_UNSIGNED_BYTE,body.data());
        zmeneno=false;
    }
};

// Program v BASICu, ktery se pise. Je to SKUTECNY funkcni listing.
const char *LISTING =
 "READY\n"
 "10 GRAPHICS 0\n"
 "20 SETCOLOR 2,12,2:SETCOLOR 4,12,2\n"
 "30 SETCOLOR 1,12,14:POKE 752,1\n"
 "40 ? :? \"    VITA VAS NEJVYSSI KKT!\"\n"
 "50 ? \n"
 "60 ? \"KDYZ MI NEKDO V DISKUSICH NA\"\n"
 "70 ? \"NETU NADAVA DO KOKOTU, JEN SE\"\n"
 "80 ? \"USMIVAM - PROTOZE VIM, ZE PRO\"\n"
 "90 ? \"VAS, MOJE KLUKY Z KLANU\"\n"
 "100 ? \"KAMARADI KRTECKA TONDY, JSEM\"\n"
 "110 ? \"OPRAVDU TEN NEJVYSSI KOKOT.\"\n"
 "120 ? \n"
 "130 ? \"TOHLE JE VASE EXKLUZIVNI BETA\"\n"
 "140 ? \"VERZE. OSTATNI LAMY SI MUSI\"\n"
 "150 ? \"POCKAT, AZ TO PUSTIM NA WEB.\"\n"
 "160 ? \n"
 "170 ? \"TAK NAHOD JOYSTICK A UKAZ,\"\n"
 "180 ? \"CO UMIS!\"\n"
 "190 ? \n"
 "200 ? \"VRACIM VAS DO ERY, KDY HRY\"\n"
 "210 ? \"MELY DUSI A OBTIZNOST,\"\n"
 "220 ? \"U KTERE BUDETE BRECET.\"\n"
 "230 ? \n"
 "240 ? \"   VENUJE VAS HERNI TATA RENE\"\n"
 "250 ? \"      A JEHO AI PARTAK\"\n"
 "260 END\n"
 "RUN\n";

// Co ten program vypise, kdyz se spusti. Presne to, co je v PRINTech.
const char *VYSTUP =
 "\n"
 "    VITA VAS NEJVYSSI KKT!\n"
 "\n"
 "KDYZ MI NEKDO V DISKUSICH NA\n"
 "NETU NADAVA DO KOKOTU, JEN SE\n"
 "USMIVAM - PROTOZE VIM, ZE PRO\n"
 "VAS, MOJE KLUKY Z KLANU\n"
 "KAMARADI KRTECKA TONDY, JSEM\n"
 "OPRAVDU TEN NEJVYSSI KOKOT.\n"
 "\n"
 "TOHLE JE VASE EXKLUZIVNI BETA\n"
 "VERZE. OSTATNI LAMY SI MUSI\n"
 "POCKAT, AZ TO PUSTIM NA WEB.\n"
 "\n"
 "TAK NAHOD JOYSTICK A UKAZ,\n"
 "CO UMIS!\n"
 "\n"
 "   VENUJE VAS HERNI TATA RENE\n"
 "      A JEHO AI PARTAK\n";

// ------------------------------------------------------------------
//  Casova osa
// ------------------------------------------------------------------
struct Scena { const char *jmeno; int trvani; };
// PRIBEH INTRA (podle Reneho predstavy):
//   zacne se na Atari, kde se PISE PROGRAM - to je zaroven napoveda,
//   protoze aplikace bude interaktivni a kdo kod nesklada, dal se
//   nedostane. Odtud se stoupa pres Segu na PS1, tam to SPADNE
//   a rozpadne se zpatky 32 -> 16 -> 8 bitu az na cernou obrazovku
//   Atari - a tam se ten program ze zacatku KONECNE SPUSTI.
const Scena SCENY[] = {
    { "zapnuti",   2500 },   // televize nabiha
    { "atari",    17000 },   // READY + listing programu (napoveda)
    { "znelkaSega", 3000 },  // nazev a znelka
    { "sega",       8000 },  // 2D cesta
    { "znelkaPs1",  3000 },
    { "ps1",        8000 },  // 3D teleso
    { "chyba",      2200 },  // spadne to
    { "rozpad",     4500 },  // 32 -> 16 -> 8 bitu
    { "beh",        9000 },  // cerna obrazovka Atari, program BEZI
    { "konec",      3000 },
};
const int POCET_SCEN = (int)(sizeof(SCENY)/sizeof(SCENY[0]));
int CELKEM_MS() { int s=0; for(int i=0;i<POCET_SCEN;i++) s+=SCENY[i].trvani; return s; }

/** Ktera scena a jak daleko v ni jsme. Vraci index, t v ms a podil 0..1 */
int scenaVCase(int cas, int *tScena, float *podil) {
    int s = 0;
    for (int i = 0; i < POCET_SCEN; i++) {
        if (cas < s + SCENY[i].trvani) {
            if (tScena) *tScena = cas - s;
            if (podil)  *podil  = (float)(cas - s) / (float)SCENY[i].trvani;
            return i;
        }
        s += SCENY[i].trvani;
    }
    if (tScena) *tScena = 0;
    if (podil)  *podil = 1.0f;
    return POCET_SCEN - 1;
}

/** Faze pro shader pozadi: 0=Atari 1=Sega 2=PS1 3=KKT, plynule mezi nimi. */
float fazeVCase(int cas) {
    int t; float p; int i = scenaVCase(cas, &t, &p);
    switch (i) {
        case 0: return 0.0f;
        case 1: return 0.0f;
        case 2: return p;                 // prechod Atari -> Sega
        case 3: return 1.0f;
        case 4: return 1.0f + p;          // Sega -> PS1
        case 5: return 2.0f;
        case 6: return 2.0f + p*0.6f;     // PS1 -> KKT
        default: return 3.0f;
    }
}

// ------------------------------------------------------------------
//  Kresleni jednoho snimku - stejne pro telefon i pro TV
// ------------------------------------------------------------------
struct Kreslic {
    GLuint progPozadi = 0, progTex = 0, progDuha = 0;
    GLuint vboCtverec = 0, vboUV = 0;
    GLint  pzCas=-1, pzRozmer=-1, pzFaze=-1, pzJas=-1, pzPos=-1;
    GLint  txPos=-1, txUV=-1, txRect=-1, txTex=-1, txAlfa=-1, txBarva=-1;
    GLint  duCas=-1, duPos=-1;
    GLuint progSega=0, progTri=0, vbo3dPos=0, vbo3dBarva=0;
    GLint  sgCas=-1, sgRozmer=-1, sgPos=-1;
    GLint  d3Pos=-1, d3Barva=-1, d3Cas=-1, d3Pomer=-1;
    int    trojuhelniku = 0;
    GLuint fbo=0, fboTex=0; int fboW=0, fboH=0;
    GLuint progRozpad=0;
    GLint  rzPos=-1, rzUV=-1, rzTex=-1, rzUrovni=-1, rzSum=-1, rzCas=-1;
    bool   pripraveno = false;
    TextObrazovka obr;
    int    napsano = 0;                 // kolik znaku listingu uz je
    bool   znelkaSpustena[10] = {false};
    bool   napisHotov[10] = {false};
    bool   behZacal = false;
    int    behNapsano = 0;

    void priprav() {
        if (pripraveno) return;
        progPozadi = program(VS, FS_POZADI);
        progTex    = program(VS_TEX, FS_TEX);
        progDuha   = program(VS, FS_DUHA);
        pzPos    = glGetAttribLocation(progPozadi, "aPos");
        pzCas    = glGetUniformLocation(progPozadi, "uCas");
        pzRozmer = glGetUniformLocation(progPozadi, "uRozmer");
        pzFaze   = glGetUniformLocation(progPozadi, "uFaze");
        pzJas    = glGetUniformLocation(progPozadi, "uJas");
        txPos   = glGetAttribLocation(progTex, "aPos");
        txUV    = glGetAttribLocation(progTex, "aUV");
        txRect  = glGetUniformLocation(progTex, "uRect");
        txTex   = glGetUniformLocation(progTex, "uTex");
        txAlfa  = glGetUniformLocation(progTex, "uAlfa");
        txBarva = glGetUniformLocation(progTex, "uBarva");
        duCas   = glGetUniformLocation(progDuha, "uCas");
        duPos   = glGetAttribLocation(progDuha, "aPos");
        const float ctverec[] = { -1,-1,  1,-1,  -1,1,   1,1 };
        glGenBuffers(1, &vboCtverec);
        glBindBuffer(GL_ARRAY_BUFFER, vboCtverec);
        glBufferData(GL_ARRAY_BUFFER, sizeof ctverec, ctverec, GL_STATIC_DRAW);
        // UV pro texturu: obraz Atari ma pocatek NAHORE, proto je V otocene
        const float uv[] = { 0,1,  1,1,  0,0,  1,0 };
        glGenBuffers(1, &vboUV);
        glBindBuffer(GL_ARRAY_BUFFER, vboUV);
        glBufferData(GL_ARRAY_BUFFER, sizeof uv, uv, GL_STATIC_DRAW);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        progSega = program(VS, FS_SEGA);
        sgPos    = glGetAttribLocation(progSega, "aPos");
        sgCas    = glGetUniformLocation(progSega, "uCas");
        sgRozmer = glGetUniformLocation(progSega, "uRozmer");

        progTri  = program(VS_3D, FS_3D);
        d3Pos    = glGetAttribLocation(progTri, "aPos");
        d3Barva  = glGetAttribLocation(progTri, "aBarva");
        d3Cas    = glGetUniformLocation(progTri, "uCas");
        d3Pomer  = glGetUniformLocation(progTri, "uPomer");
        postavKrychli();
        progRozpad = program(VS_TEX, FS_ROZPAD);
        rzPos    = glGetAttribLocation(progRozpad, "aPos");
        rzUV     = glGetAttribLocation(progRozpad, "aUV");
        rzTex    = glGetUniformLocation(progRozpad, "uTex");
        rzUrovni = glGetUniformLocation(progRozpad, "uUrovni");
        rzSum    = glGetUniformLocation(progRozpad, "uSum");
        rzCas    = glGetUniformLocation(progRozpad, "uCas");
        pripraveno = true;
        INTRO_LOG("KRESLIC PRIPRAVEN");
    }

    /** Vlastni framebuffer pro rozpad. Rozliseni se meni podle toho,
        jak hluboko uz jsme - proto se muze prestavovat za behu. */
    void pripravFbo(int w, int h) {
        if (fbo && fboW == w && fboH == h) return;
        if (fboTex) { glDeleteTextures(1,&fboTex); fboTex=0; }
        if (fbo)    { glDeleteFramebuffers(1,&fbo); fbo=0; }
        fboW = w; fboH = h;
        glGenTextures(1,&fboTex);
        glBindTexture(GL_TEXTURE_2D, fboTex);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,w,h,0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        glGenFramebuffers(1,&fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, fboTex, 0);
        GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (st != GL_FRAMEBUFFER_COMPLETE) INTRO_LOG("FBO neni hotovy: 0x%x", st);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    /** Krychle pro scenu PS1 - 6 sten po dvou trojuhelnicich. */
    void postavKrychli() {
        static const float v[8][3] = {
            {-1,-1,-1},{ 1,-1,-1},{ 1, 1,-1},{-1, 1,-1},
            {-1,-1, 1},{ 1,-1, 1},{ 1, 1, 1},{-1, 1, 1} };
        static const int steny[6][4] = {
            {0,1,2,3},{5,4,7,6},{4,0,3,7},{1,5,6,2},{4,5,1,0},{3,2,6,7} };
        static const float barvy[6][3] = {
            {0.95f,0.20f,0.25f},{0.20f,0.55f,0.95f},{0.95f,0.75f,0.15f},
            {0.25f,0.85f,0.45f},{0.75f,0.25f,0.90f},{0.95f,0.95f,0.95f} };
        std::vector<float> pos, bar;
        for (int f=0; f<6; f++) {
            const int idx[6] = { steny[f][0],steny[f][1],steny[f][2],
                                 steny[f][0],steny[f][2],steny[f][3] };
            for (int k=0;k<6;k++) {
                pos.push_back(v[idx[k]][0]*0.80f);
                pos.push_back(v[idx[k]][1]*0.80f);
                pos.push_back(v[idx[k]][2]*0.80f);
                bar.push_back(barvy[f][0]); bar.push_back(barvy[f][1]); bar.push_back(barvy[f][2]);
            }
        }
        trojuhelniku = (int)(pos.size()/3);
        glGenBuffers(1,&vbo3dPos);
        glBindBuffer(GL_ARRAY_BUFFER, vbo3dPos);
        glBufferData(GL_ARRAY_BUFFER, pos.size()*sizeof(float), pos.data(), GL_STATIC_DRAW);
        glGenBuffers(1,&vbo3dBarva);
        glBindBuffer(GL_ARRAY_BUFFER, vbo3dBarva);
        glBufferData(GL_ARRAY_BUFFER, bar.size()*sizeof(float), bar.data(), GL_STATIC_DRAW);
    }

    /** Obdelnik s texturou. Souradnice ve zlomcich obrazovky 0..1. */
    void obdelnikTex(GLuint tex, float x, float y, float sir, float vys,
                     float alfa, float br, float bg, float bb) {
        glUseProgram(progTex);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(txTex, 0);
        glUniform1f(txAlfa, alfa);
        glUniform3f(txBarva, br, bg, bb);
        // prevod 0..1 na -1..1
        glUniform4f(txRect, (x+sir*0.5f)*2.f-1.f, 1.f-(y+vys*0.5f)*2.f, sir, vys);
        glBindBuffer(GL_ARRAY_BUFFER, vboCtverec);
        glEnableVertexAttribArray(txPos);
        glVertexAttribPointer(txPos, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glBindBuffer(GL_ARRAY_BUFFER, vboUV);
        glEnableVertexAttribArray(txUV);
        glVertexAttribPointer(txUV, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisableVertexAttribArray(txPos);
        glDisableVertexAttribArray(txUV);
    }

    /** Kam se vejde obrazovka Atari 4:3 - na vysku i na sirku. */
    void mistoProObrazovku(int w, int h, float *x, float *y, float *sir, float *vys) {
        float pomer = (float)OBR_W / (float)OBR_H;      // 320/192 = 1,667
        // na obrazovce ma bod Atari pomer 0,8 -> vysledek 4:3
        pomer *= 0.8f;
        float dostupnaS = 0.86f, dostupnaV = 0.72f;
        float sirBod = dostupnaS * w, vysBod = dostupnaV * h;
        if (sirBod / vysBod > pomer) sirBod = vysBod * pomer;
        else                          vysBod = sirBod / pomer;
        *sir = sirBod / w;  *vys = vysBod / h;
        *x = (1.f - *sir) * 0.5f;
        *y = (1.f - *vys) * 0.5f;
    }

    /**
     * Snimek. Pri rozpadu se scena nejdriv nakresli do VLASTNI TEXTURY
     * v cim dal mensim rozliseni a teprve pak se roztahne pres obrazovku -
     * odtud ty kosticky. Jinak se kresli rovnou na obrazovku.
     */
    void snimek(int cas, int w, int h) {
        int tS; float p;
        int i = scenaVCase(cas, &tS, &p);

        if (i == 7) {                       // rozpad
            // 1 -> 1/12 rozliseni podle toho, jak hluboko uz jsme
            int del = 1 + (int)(p * 11.f);
            int mw = w/del < 32 ? 32 : w/del;
            int mh = h/del < 32 ? 32 : h/del;
            pripravFbo(mw, mh);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            kresliScenu(cas, mw, mh, 5);    // porad se kresli PS1 teleso
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            kresliScenu(cas, w, h, i);      // a tady uz jen roztazeni
            return;
        }
        kresliScenu(cas, w, h, i);
    }

    void kresliScenu(int cas, int w, int h, int i) {
        int tS; float p;
        scenaVCase(cas, &tS, &p);

        glViewport(0, 0, w, h);
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 1. scena: televize se zapina - jen bila cara, ktera se rozevre
        if (i == 0) {
            float q = p;
            glEnable(GL_SCISSOR_TEST);
            int vys = (int)(h * (q < 0.35f ? 0.004f : (q-0.35f)*(q-0.35f)*2.2f));
            if (vys < 2) vys = 2;
            int sir = (int)(w * (q < 0.30f ? q/0.30f : 1.0f));
            glScissor((w-sir)/2, h/2 - vys/2, sir, vys);
            float j = (q < 0.35f) ? 1.0f : (1.0f - (q-0.35f)*1.2f);
            if (j < 0.f) j = 0.f;
            glClearColor(j, j, j, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_SCISSOR_TEST);
            return;
        }

        // pozadi na GPU
        float jas = 1.0f;
        if (i == POCET_SCEN-1) jas = 1.0f - p;           // zatmivacka
        glUseProgram(progPozadi);
        glUniform1f(pzCas, cas / 1000.0f);
        glUniform2f(pzRozmer, (float)w, (float)h);
        glUniform1f(pzFaze, fazeVCase(cas));
        glUniform1f(pzJas, jas);
        glBindBuffer(GL_ARRAY_BUFFER, vboCtverec);
        glEnableVertexAttribArray(pzPos);
        glVertexAttribPointer(pzPos, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glDisableVertexAttribArray(pzPos);

        // ---- 2. scena: ATARI ----
        if (i == 1) {
            const unsigned char *font = NAP_OS_ROM + 0x2000;
            const int PAUZA = 1200;          // READY chvili jen sviti
            const int RYCH  = 34;            // ms na znak
            int delka = (int)strlen(LISTING);
            int chce = (tS < PAUZA) ? 5 : (5 + (tS - PAUZA) / RYCH);   // "READY" hned
            if (chce > delka) chce = delka;
            while (napsano < chce) {
                char z = LISTING[napsano];
                obr.pis(z);
                if (napsano >= 6) g_zvuk.klapnuti(z == '\n');   // "READY" vypisuje BASIC, neklape
                napsano++;
            }

            bool dopsano = (napsano >= delka);
            bool kurzor  = ((cas / 330) % 2) == 0;
            obr.nahraj(font, 214, 224, 255, true, kurzor && !dopsano);

            float x,y,sir,vys; mistoProObrazovku(w,h,&x,&y,&sir,&vys);
            // ramecek monitoru
            glUseProgram(progPozadi);
            // modre pozadi obrazovky nakreslime pres texturu s plnou alfou:
            // staci nakreslit tmavy obdelnik pod ni pomoci nuzek
            glEnable(GL_SCISSOR_TEST);
            glScissor((int)(x*w), (int)((1.f-y-vys)*h), (int)(sir*w), (int)(vys*h));
            if (dopsano) { glClearColor(0.11f,0.36f,0.08f,1.f); }   // po RUN: zelena
            else         { glClearColor(0.23f,0.29f,0.78f,1.f); }   // GRAPHICS 0 modra
            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_SCISSOR_TEST);

            obdelnikTex(obr.tex, x, y, sir, vys, 1.f, 1.f, 1.f, 1.f);
        }

        // ---- SEGA: 2D cesta ----
        if (i == 3) {
            glUseProgram(progSega);
            glUniform1f(sgCas, cas/1000.0f);
            glUniform2f(sgRozmer, (float)w, (float)h);
            glBindBuffer(GL_ARRAY_BUFFER, vboCtverec);
            glEnableVertexAttribArray(sgPos);
            glVertexAttribPointer(sgPos, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glDisableVertexAttribArray(sgPos);
        }

        // ---- PS1: otacejici se teleso, skutecne 3D ----
        if (i == 5 || i == 6) {
            glEnable(GL_DEPTH_TEST);
            glClear(GL_DEPTH_BUFFER_BIT);
            glUseProgram(progTri);
            glUniform1f(d3Cas, cas/1000.0f);
            glUniform1f(d3Pomer, (float)w/(float)h);
            glBindBuffer(GL_ARRAY_BUFFER, vbo3dPos);
            glEnableVertexAttribArray(d3Pos);
            glVertexAttribPointer(d3Pos, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glBindBuffer(GL_ARRAY_BUFFER, vbo3dBarva);
            glEnableVertexAttribArray(d3Barva);
            glVertexAttribPointer(d3Barva, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glDrawArrays(GL_TRIANGLES, 0, trojuhelniku);
            glDisableVertexAttribArray(d3Pos);
            glDisableVertexAttribArray(d3Barva);
            glDisable(GL_DEPTH_TEST);
        }

        // ---- ZNELKY: vlastni napis na cerne, jako kdyz nabihá stroj ----
        if (i == 2 || i == 4) {
            if (!znelkaSpustena[i]) { znelkaSpustena[i] = true; g_zvuk.znelka(i == 2 ? 0 : 1); }
            float q = p;
            float sila = (q < 0.22f) ? q/0.22f : (q > 0.80f ? (1.f-q)/0.20f : 1.f);

            // pas, ktery se rozjede odspodu a zase zhasne
            glEnable(GL_SCISSOR_TEST);
            int pas = (int)(h * 0.34f * sila);
            glScissor(0, h/2 - pas/2, w, pas);
            if (i == 2) glClearColor(0.05f*sila, 0.18f*sila, 0.55f*sila, 1.f);
            else        glClearColor(0.30f*sila, 0.28f*sila, 0.33f*sila, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_SCISSOR_TEST);

            // vlastni napis - vysazeny pismem z ROM do textove obrazovky
            if (!napisHotov[i]) {
                napisHotov[i] = true;
                obr.vymaz();
                for (int r = 0; r < 11; r++) obr.pis('\n');
                const char *t = (i == 2) ? "            MEGA DRIVE\n"
                                         : "           PLAYSTATION\n";
                for (const char *c = t; *c; c++) obr.pis(*c);
                const char *t2 = (i == 2) ? "         EMULOVANO V EMU10\n"
                                          : "         EMULOVANO V EMU10\n";
                for (const char *c = t2; *c; c++) obr.pis(*c);
            }
            obr.nahraj(NAP_OS_ROM + 0x2000,
                       (int)(255*sila), (int)(255*sila), (int)(255*sila), false, false);
            float x,y,sir,vys; mistoProObrazovku(w,h,&x,&y,&sir,&vys);
            obdelnikTex(obr.tex, x, y, sir, vys, sila, 1.f, 1.f, 1.f);
        }

        // ---- CHYBA: obraz se rozsype ----
        if (i == 6) {
            glEnable(GL_SCISSOR_TEST);
            for (int k = 0; k < 14; k++) {
                int yy = (int)((sinf(cas*0.09f + k*1.7f)*0.5f+0.5f) * h);
                int vys = 4 + (k*7) % 26;
                int posun = (int)(sinf(cas*0.21f + k*2.3f) * w * 0.35f * p);
                glScissor(posun, yy, w, vys);
                float j = (k%3==0) ? 1.f : 0.f;
                glClearColor(j, j*0.2f, j*0.2f, 1.f);
                glClear(GL_COLOR_BUFFER_BIT);
            }
            glDisable(GL_SCISSOR_TEST);
        }

        // ---- ROZPAD: 32 -> 16 -> 8 bitu ----
        // Scena uz je nakreslena do FBO (viz snimekDoFbo). Tady se jen
        // roztahne pres obrazovku s mensim poctem odstinu a kostickami.
        if (i == 7) {
            glUseProgram(progRozpad);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, fboTex);
            glUniform1i(rzTex, 0);
            // 256 odstinu -> 32 -> 8 -> 4
            float urovni = 256.f * powf(0.02f, p);
            if (urovni < 3.f) urovni = 3.f;
            glUniform1f(rzUrovni, urovni);
            glUniform1f(rzSum, p);
            glUniform1f(rzCas, cas/1000.0f);
            glUniform4f(glGetUniformLocation(progRozpad,"uRect"), 0.f, 0.f, 1.f, 1.f);
            glBindBuffer(GL_ARRAY_BUFFER, vboCtverec);
            glEnableVertexAttribArray(rzPos);
            glVertexAttribPointer(rzPos, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            glBindBuffer(GL_ARRAY_BUFFER, vboUV);
            glEnableVertexAttribArray(rzUV);
            glVertexAttribPointer(rzUV, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glDisableVertexAttribArray(rzPos);
            glDisableVertexAttribArray(rzUV);
        }

        // ---- BEH: cerna obrazovka Atari a program KONECNE BEZI ----
        if (i == 8) {
            const unsigned char *font = NAP_OS_ROM + 0x2000;
            // obrazovku mezitim pouzily znelky, takze zaciname nacisto
            if (!behZacal) { behZacal = true; obr.vymaz(); behNapsano = 0; }
            const int RYCH = 42;
            int delka = (int)strlen(VYSTUP);
            int chce = tS / RYCH; if (chce > delka) chce = delka;
            while (behNapsano < chce) { obr.pis(VYSTUP[behNapsano]); behNapsano++; }
            obr.nahraj(font, 255, 255, 255, true, ((cas/330)%2)==0 && behNapsano<delka);

            float x,y,sir,vys; mistoProObrazovku(w,h,&x,&y,&sir,&vys);
            // duha pod textem - na Atari se dela zmenou barvy na kazdem radku
            glUseProgram(progDuha);
            glUniform1f(duCas, cas/1000.0f);
            glEnable(GL_SCISSOR_TEST);
            glScissor((int)(x*w), (int)((1.f-y-vys)*h), (int)(sir*w), (int)(vys*h));
            glBindBuffer(GL_ARRAY_BUFFER, vboCtverec);
            glEnableVertexAttribArray(duPos);
            glVertexAttribPointer(duPos, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glDisableVertexAttribArray(duPos);
            glDisable(GL_SCISSOR_TEST);

            obdelnikTex(obr.tex, x, y, sir, vys, 1.f, 1.f, 1.f, 1.f);
        }
    }
};

// ------------------------------------------------------------------
//  Spolecne vlakno pro obe plochy
// ------------------------------------------------------------------
static void vlaknoKresli(ANativeWindow *win, std::atomic<bool> *bezi, const char *jmeno, bool proTv) {
    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint ma=0, mi=0;
    if (dpy == EGL_NO_DISPLAY || !eglInitialize(dpy, &ma, &mi)) {
        INTRO_LOG("%s: EGL se nepodarilo otevrit", jmeno);
        ANativeWindow_release(win); return;
    }
    const EGLint cfga[] = { EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                            EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
                            EGL_NONE };
    EGLConfig cfg; EGLint n=0;
    if (!eglChooseConfig(dpy, cfga, &cfg, 1, &n) || n < 1) {
        INTRO_LOG("%s: zadna vhodna konfigurace", jmeno);
        ANativeWindow_release(win); return;
    }
    EGLint fmt=0; eglGetConfigAttrib(dpy, cfg, EGL_NATIVE_VISUAL_ID, &fmt);
    ANativeWindow_setBuffersGeometry(win, 0, 0, fmt);

    EGLSurface surf = eglCreateWindowSurface(dpy, cfg, (EGLNativeWindowType)win, nullptr);
    const EGLint ctxa[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxa);
    if (surf == EGL_NO_SURFACE || ctx == EGL_NO_CONTEXT ||
        !eglMakeCurrent(dpy, surf, surf, ctx)) {
        INTRO_LOG("%s: kontext se nepodarilo pripravit (0x%x)", jmeno, eglGetError());
        ANativeWindow_release(win); return;
    }

    Kreslic k; k.priprav();
    INTRO_LOG("%s: PLOCHA PRIPRAVENA", jmeno);

    const int celkem = CELKEM_MS();
    while (bezi->load()) {
        int w=0, h=0;
        eglQuerySurface(dpy, surf, EGL_WIDTH,  &w);
        eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
        if (w <= 0 || h <= 0) { std::this_thread::sleep_for(std::chrono::milliseconds(16)); continue; }

        long long zac = g_start_ms.load();
        int cas = (zac > 0) ? (int)(ted_ms() - zac) : 0;
        if (cas >= celkem) { g_hotovo.store(true); cas = celkem - 1; }

        k.snimek(cas, w, h);

        // Snimek pro TV se bere JEN z telefonni plochy a jen kdyz si
        // ho nekdo vyzvedl - jinak by se cetlo zbytecne.
        if (proTv && g_tv_chce.load()) {
            const int tw = w > 640 ? 640 : w;               // TV nepotrebuje vic
            const int th = (int)((long long)h * tw / w);
            std::lock_guard<std::mutex> zamek(g_tv_zamek);
            if ((int)g_tv_body.size() < tw*th*4) g_tv_body.resize((size_t)tw*th*4);
            glReadPixels(0, 0, tw, th, GL_RGBA, GL_UNSIGNED_BYTE, g_tv_body.data());
            g_tv_w = tw; g_tv_h = th; g_tv_novy = true;
        }

        eglSwapBuffers(dpy, surf);
        g_snimku.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(8));
    }

    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(dpy, surf);
    eglDestroyContext(dpy, ctx);
    ANativeWindow_release(win);
    INTRO_LOG("%s: PLOCHA UKONCENA", jmeno);
}

} // namespace

// ====================================================================
//  JNI
// ====================================================================
extern "C" {

JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeIntroBridge_introStart(JNIEnv *, jclass) {
    g_start_ms.store(ted_ms());
    g_hotovo.store(false);
    g_snimku.store(0);
    g_vzorku.store(0);
    g_zvuk.start();
    INTRO_LOG("INTRO START");
}

JNIEXPORT jboolean JNICALL
Java_eu_atarihelp_emu10_NativeIntroBridge_introHotovo(JNIEnv *, jclass) {
    return g_hotovo.load() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jlong JNICALL
Java_eu_atarihelp_emu10_NativeIntroBridge_introSnimku(JNIEnv *, jclass) {
    return (jlong)g_snimku.load();
}

JNIEXPORT jlong JNICALL
Java_eu_atarihelp_emu10_NativeIntroBridge_introVzorku(JNIEnv *, jclass) {
    return (jlong)g_vzorku.load();
}

/** Java si sem chodi pro vzorky. Vraci pocet naplnenych ramcu. */
JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeIntroBridge_introNaplnZvuk(JNIEnv *env, jclass,
                                                         jshortArray jbuf, jint ramcu) {
    if (jbuf == nullptr || ramcu <= 0) return 0;
    jshort *p = env->GetShortArrayElements(jbuf, nullptr);
    if (!p) return 0;
    long long zac = g_start_ms.load();
    int cas = (zac > 0) ? (int)(ted_ms() - zac) : 0;
    g_zvuk.naplni((short *)p, ramcu, cas);
    env->ReleaseShortArrayElements(jbuf, p, 0);
    g_vzorku.fetch_add(ramcu);
    return ramcu;
}

/**
 * Nahraje znelku ze ZARIZENI. Java precte soubor a preda vzorky.
 * ktera: 0 = misto Segy, 1 = misto PS1.
 * Soubor NENI soucasti aplikace - lezi na telefonu.
 */
JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeIntroBridge_introNactiZnelku(JNIEnv *env, jclass, jint ktera,
                                                           jshortArray jvz, jint vzorkovani) {
    if (jvz == nullptr) return;
    jsize n = env->GetArrayLength(jvz);
    jshort *p = env->GetShortArrayElements(jvz, nullptr);
    if (!p) return;
    g_zvuk.nastavNahravku(ktera, (const short *)p, (int)n, vzorkovani);
    env->ReleaseShortArrayElements(jvz, p, JNI_ABORT);
    INTRO_LOG("ZNELKA %d nactena ze zarizeni: %d vzorku, %d Hz", (int)ktera, (int)n, (int)vzorkovani);
}

/**
 * Pujci posledni snimek Jave pro TV - STEJNE jako Ps1GlTextureView.borrowFrame
 * a NativeSegaCoreBridge.grabFrame.
 *   vraci  (sirka<<16)|vyska   kdyz snimek predal
 *          zaporne             kdyz je pole male (potreba tolik bodu)
 *          0                   kdyz nic noveho neni
 */
JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativeIntroBridge_introGrabFrame(JNIEnv *env, jclass, jintArray jven) {
    g_tv_chce.store(true);                       // od ted snimky odkladame
    std::lock_guard<std::mutex> zamek(g_tv_zamek);
    if (!g_tv_novy || g_tv_w <= 0 || g_tv_h <= 0) return 0;
    const int potreba = g_tv_w * g_tv_h;
    if (jven == nullptr || env->GetArrayLength(jven) < potreba)
        return -((g_tv_w << 16) | g_tv_h);
    jint *ven = env->GetIntArrayElements(jven, nullptr);
    if (!ven) return 0;
    // OpenGL cte zdola nahoru, obraz musime otocit; a RGBA -> ARGB
    for (int y = 0; y < g_tv_h; y++) {
        const unsigned char *r = &g_tv_body[(size_t)(g_tv_h-1-y) * g_tv_w * 4];
        jint *w2 = ven + (size_t)y * g_tv_w;
        for (int x = 0; x < g_tv_w; x++) {
            w2[x] = (jint)(0xFF000000u | ((unsigned)r[x*4] << 16)
                                       | ((unsigned)r[x*4+1] << 8)
                                       |  (unsigned)r[x*4+2]);
        }
    }
    env->ReleaseIntArrayElements(jven, ven, 0);
    g_tv_novy = false;
    return (jint)((g_tv_w << 16) | g_tv_h);
}

/** Klapnuti klavesy - vola scena Atari, kdyz pribude znak. */
JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeIntroBridge_introKlapnuti(JNIEnv *, jclass, jboolean konecRadku) {
    g_zvuk.klapnuti(konecRadku == JNI_TRUE);
}


JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeIntroBridge_introSetDisplaySurface(JNIEnv *env, jclass, jobject jsurf) {
    if (g_disp_run.exchange(false) && g_disp_thread.joinable()) g_disp_thread.join();
    g_disp_win = nullptr;
    if (jsurf == nullptr) { INTRO_LOG("PLOCHA: odpojena"); return; }
    ANativeWindow *win = ANativeWindow_fromSurface(env, jsurf);
    if (!win) { INTRO_LOG("PLOCHA: okno se nepodarilo ziskat"); return; }
    g_disp_win = win;
    g_disp_run.store(true);
    g_disp_thread = std::thread(vlaknoKresli, win, &g_disp_run, "PLOCHA", true);
    INTRO_LOG("PLOCHA: pripojena");
}

JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativeIntroBridge_introSetTvSurface(JNIEnv *env, jclass, jobject jsurf) {
    /* Stejne jako u PS1: NECEKAME na stare vlakno - Java sem muze prijit
       z mista, kde uz drzi zamek enkoderu, a cekanim by obe strany stály. */
    bool bezelo = g_tv_run.exchange(false);
    if (bezelo && g_tv_thread.joinable()) g_tv_thread.detach();
    g_tv_win = nullptr;
    if (jsurf == nullptr) { INTRO_LOG("TV: odpojeno"); return; }
    ANativeWindow *win = ANativeWindow_fromSurface(env, jsurf);
    if (!win) { INTRO_LOG("TV: okno se nepodarilo ziskat"); return; }
    g_tv_win = win;
    g_tv_run.store(true);
    g_tv_thread = std::thread(vlaknoKresli, win, &g_tv_run, "TV", false);
    INTRO_LOG("TV: pripojeno, kreslim primo do enkoderu");
}

} // extern "C"
