// test_overeni/b313/tv_menu_lib_b313.cpp
// ====================================================================
//  B313: SKUTECNY nap_menu_gl.cpp jako knihovna pro test prohlizece (jako ../b311/tv_menu_lib.cpp)
//  + menuNastav (obnova displeje, kvalita TV -> velikost obrazu menu na TV) a snimek menu
//  pro TV, ze ktereho jde proud (jako menuGrabFrame) - test s nim porovna canvas prohlizece.
//
//   g++ -std=c++17 -O2 -shared -fPIC -pthread -w -I../b304/stub -I../b291/stub -I$INC
//       -I$JAVA_HOME/include -I$JAVA_HOME/include/linux -I../../app/src/main/cpp/tvproud
//       -o libmenu_b313.so tv_menu_lib_b313.cpp -l:libEGL.so.1 -l:libGLESv2.so.2
// ====================================================================
#include "../b311/tv_menu_lib.cpp"

extern "C" {

/** jako Java NativeMenuBridge.nastavSafe: obnovovaci frekvence displeje, kvalita TV 0 / 1 / 2 */
void menu_nastav_c(float hz, int kvalita) { Java_eu_atarihelp_emu10_NativeMenuBridge_menuNastav(&g_env, nullptr, hz, kvalita); }

/**
 * posledni snimek menu pro TV (0x00RRGGBB) do ven (kap bodu): (W<<16)|H, 0 = jeste nic,
 * zaporne = male pole. Stejne jako menuGrabFrame, ale bez falesneho JNIEnv (int[] / byte[]
 * se tam rozlisuje spolecnym priznakem - proud bezi soubezne v jinem vlakne).
 */
int menu_grab_c(uint32_t *ven, int kap) {
  g_tvChceNs.store(tedNs());
  std::lock_guard<std::mutex> z(g_tvZamek);
  const int W = g_tvBodyW, H = g_tvBodyH;
  if (!g_tvNovy || W <= 0 || H <= 0 || g_tvBody.size() < (size_t)W * H * 4) return 0;
  if (kap < W * H) return -((W << 16) | H);
  for (int y = 0; y < H; y++) {
    const unsigned char *r = &g_tvBody[(size_t)(H - 1 - y) * W * 4];
    for (int x = 0; x < W; x++) ven[(size_t)y * W + x] = ((uint32_t)r[x * 4] << 16) | ((uint32_t)r[x * 4 + 1] << 8) | (uint32_t)r[x * 4 + 2];
  }
  g_tvNovy = false;
  return (W << 16) | H;
}

}  // extern "C"
