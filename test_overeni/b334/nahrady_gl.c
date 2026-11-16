/* B334: nahrady EGL / OpenGL ES / zlib pro test jadra PS1 na 32bitovem ARM v qemu-arm (Linux armhf nema
 * knihovny Androidu). Obraz testu se bere z jadra (snimek do pameti), ne z OpenGL - EGL se "nepovede"
 * (appka to umi: kresli bez GLES), zlib se nepouzije (kresleni_test.exe neni CHD ani komprimovany obraz). */
#include <stddef.h>
#include <stdint.h>
typedef void *P;
typedef unsigned int U;
/* EGL: vse selze (EGL_NO_DISPLAY / EGL_FALSE), chyba EGL_NOT_INITIALIZED */
P eglGetDisplay(P d) { (void)d; return 0; }
U eglInitialize(P d, int *a, int *b) { (void)d; (void)a; (void)b; return 0; }
U eglChooseConfig(P d, const int *a, P *c, int n, int *m) { (void)d; (void)a; (void)c; (void)n; if (m) *m = 0; return 0; }
P eglCreateContext(P d, P c, P s, const int *a) { (void)d; (void)c; (void)s; (void)a; return 0; }
P eglCreatePbufferSurface(P d, P c, const int *a) { (void)d; (void)c; (void)a; return 0; }
P eglCreateWindowSurface(P d, P c, P w, const int *a) { (void)d; (void)c; (void)w; (void)a; return 0; }
U eglDestroyContext(P d, P c) { (void)d; (void)c; return 0; }
U eglDestroySurface(P d, P s) { (void)d; (void)s; return 0; }
U eglGetConfigAttrib(P d, P c, int a, int *v) { (void)d; (void)c; (void)a; if (v) *v = 0; return 0; }
P eglGetCurrentContext(void) { return 0; }
P eglGetCurrentSurface(int r) { (void)r; return 0; }
int eglGetError(void) { return 0x3001; }
U eglMakeCurrent(P d, P a, P b, P c) { (void)d; (void)a; (void)b; (void)c; return 0; }
/* GLES2: nic */
void glAttachShader(U p, U s) { (void)p; (void)s; }
void glBindTexture(U t, U x) { (void)t; (void)x; }
void glCompileShader(U s) { (void)s; }
U glCreateProgram(void) { return 0; }
U glCreateShader(U t) { (void)t; return 0; }
void glDeleteShader(U s) { (void)s; }
void glGenTextures(int n, U *t) { for (int i = 0; i < n; i++) t[i] = 0; }
int glGetAttribLocation(U p, const char *n) { (void)p; (void)n; return -1; }
void glGetShaderInfoLog(U s, int m, int *l, char *b) { (void)s; if (l) *l = 0; if (m > 0 && b) b[0] = 0; }
void glGetShaderiv(U s, U p, int *v) { (void)s; (void)p; if (v) *v = 0; }
int glGetUniformLocation(U p, const char *n) { (void)p; (void)n; return -1; }
void glLinkProgram(U p) { (void)p; }
void glShaderSource(U s, int n, const char *const *t, const int *l) { (void)s; (void)n; (void)t; (void)l; }
void glTexParameteri(U t, U p, int v) { (void)t; (void)p; (void)v; }
/* zlib: chyba (Z_STREAM_ERROR) */
unsigned long crc32(unsigned long c, const unsigned char *b, unsigned n) { (void)b; (void)n; return c; }
P gzopen64(const char *p, const char *m) { (void)p; (void)m; return 0; }
int gzclose(P f) { (void)f; return -2; }
int gzread(P f, P b, unsigned n) { (void)f; (void)b; (void)n; return -1; }
int gzwrite(P f, const void *b, unsigned n) { (void)f; (void)b; (void)n; return 0; }
long long gzseek64(P f, long long o, int w) { (void)f; (void)o; (void)w; return -1; }
int inflate(P s, int f) { (void)s; (void)f; return -2; }
int inflateEnd(P s) { (void)s; return -2; }
int inflateInit2_(P s, int w, const char *v, int z) { (void)s; (void)w; (void)v; (void)z; return -2; }
int inflateReset(P s) { (void)s; return -2; }
int uncompress(unsigned char *d, unsigned long *dl, const unsigned char *s, unsigned long sl) { (void)d; (void)dl; (void)s; (void)sl; return -2; }
