// B307: 32bitova ukazka na PC (Mesa, OpenGL ES 2.0 bez okna) - snimky do PPM
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "../../app/src/main/cpp/atari/nap_atari_roms.h"
#include "../../app/src/main/cpp/intro/nap_demo_16bit.h"
#include "../../app/src/main/cpp/intro/nap_demo_32bit.h"
static bool glKontext() {
  auto getPD = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
  EGLDisplay d = getPD ? getPD(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr) : EGL_NO_DISPLAY;
  if (d == EGL_NO_DISPLAY) return false;
  EGLint ma, mi;
  if (!eglInitialize(d, &ma, &mi)) return false;
  eglBindAPI(EGL_OPENGL_ES_API);
  const EGLint xa[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  EGLContext x = eglCreateContext(d, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, xa);
  return x != EGL_NO_CONTEXT && eglMakeCurrent(d, EGL_NO_SURFACE, EGL_NO_SURFACE, x);
}
int main(int argc, char **argv) {
  const std::string adr = argc > 1 ? argv[1] : ".";
  if (!glKontext()) { std::fprintf(stderr, "EGL ne\n"); return 1; }
  uint8_t pismo[96][8]; napdemo::pismoTucne(pismo);
  napdemo::Hra32 h;
  auto t0 = std::chrono::steady_clock::now();
  bool ok = h.init(&pismo[0][0]);
  auto t1 = std::chrono::steady_clock::now();
  std::printf("init %s %.1f ms\n", ok ? "OK" : "SELHAL", std::chrono::duration<double, std::milli>(t1 - t0).count());
  std::vector<unsigned char> px(320 * 240 * 4);
  for (int i = 2; i < argc; i++) {
    const float t = (float)std::atof(argv[i]);
    auto a = std::chrono::steady_clock::now();
    h.snimek(t);
    glBindFramebuffer(GL_FRAMEBUFFER, h.fbo.fbo);
    glReadPixels(0, 0, 320, 240, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    auto b = std::chrono::steady_clock::now();
    std::printf("t=%.2f %.1f ms chyba GL 0x%x\n", t, std::chrono::duration<double, std::milli>(b - a).count(), glGetError());
    char jm[512]; std::snprintf(jm, sizeof jm, "%s/u32_%05.2f.ppm", adr.c_str(), t);
    FILE *o = std::fopen(jm, "wb");
    std::fprintf(o, "P6\n320 240\n255\n");
    for (int y = 239; y >= 0; y--) for (int x = 0; x < 320; x++) std::fwrite(&px[(size_t)(y * 320 + x) * 4], 1, 3, o);
    std::fclose(o);
  }
}
