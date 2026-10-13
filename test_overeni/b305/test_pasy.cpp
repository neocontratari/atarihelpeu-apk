// B305: koder JPEG - pasy soubezne (nap_tv_proud_jpegenc.h). Pocet pasu se meni
// kazdy snimek (1, 2, 3, 2, 3, 1 ...) a snimky se stridaji - vysledek musi byt bajt
// po bajtu stejny jako pri prvnim zakodovani se stejnym poctem pasu. Pod
// ThreadSanitizerem overi i vlakna pomocniku.
//   g++ -std=c++17 -O2 -pthread -I../../app/src/main/cpp/tvproud -o test_pasy test_pasy.cpp
//   (ThreadSanitizer: -O1 -g -fsanitize=thread)
#include <cstdio>
#include <vector>
#include "nap_tv_proud_jpegenc.h"
int main() {
  const int W = 640, H = 480;
  std::vector<uint32_t> F[2];
  for (int k = 0; k < 2; k++) {
    F[k].resize((size_t)W * H);
    for (int y = 0; y < H; y++)
      for (int x = 0; x < W; x++)
        F[k][(size_t)y * W + x] = ((uint32_t)((x * 3 + y + k * 40) & 255) << 16) | ((uint32_t)((y * 2 + k * 90) & 255) << 8) | (uint32_t)(((x ^ y) + k * 17) & 255);
  }
  nap::proud::JpegEnc e;
  std::vector<uint8_t> vzor[2][4];
  for (int k = 0; k < 2; k++)
    for (int c = 1; c <= 3; c++) e.koduj(F[k].data(), W, W, H, 80, vzor[k][c], c);
  const int poradi[] = {1, 2, 3, 2, 3, 1, 3, 1, 2};
  int spatne = 0, kol = 0;
  for (int i = 0; i < 300; i++) {
    const int c = poradi[i % 9], k = (i / 2) % 2;
    std::vector<uint8_t> j;
    e.koduj(F[k].data(), W, W, H, 80, j, c);
    if (j != vzor[k][c]) spatne++;
    kol++;
  }
  // castecny obdelnik (jiny stride nez sirka) se 2 a 3 pasy
  std::vector<uint8_t> a, b;
  e.koduj(F[0].data() + 64 * W + 32, W, 400, 300, 80, a, 1);
  e.koduj(F[0].data() + 64 * W + 32, W, 400, 300, 80, b, 1);
  printf("%d zakodovani (pasy 1/2/3 stridave, 2 snimky): %d jinych nez vzor; obdelnik 400x300 opakovane stejny: %s\n", kol, spatne, a == b ? "ANO" : "NE");
  printf("VYSLEDEK: %s\n", spatne == 0 && a == b ? "OK" : "CHYBA");
  return spatne == 0 && a == b ? 0 : 1;
}
