// B306: pomocna vlakna koderu JPEG (pasy na vic jadrech) bezi s prioritou jako
// Android THREAD_PRIORITY_DISPLAY (nice -4) - nap_tv_proud_spol.h zvysPrioritu().
// Overeni pres /proc/self/task/<tid>/stat (pole 19 = nice). Na PC jako root (kontejner)
// se zmena povoli, jinak setpriority vrati chybu a nic se nestane (zkontroluje se i to).
//   g++ -std=c++17 -O2 -pthread -I../../app/src/main/cpp/tvproud -o test_priorita test_priorita.cpp
#include "nap_tv_proud_jpegenc.h"
#include <dirent.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
using namespace nap::proud;
static int niceVlakna(const std::string &tid) {
  std::ifstream f("/proc/self/task/" + tid + "/stat");
  std::string s((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  const size_t z = s.rfind(')');
  std::istringstream is(s.substr(z + 2));
  std::string pole;
  for (int i = 3; i <= 19; i++) is >> pole;         // pole 19 = nice
  return std::stoi(pole);
}
int main() {
  std::vector<uint32_t> F(640 * 480);
  for (size_t i = 0; i < F.size(); i++) F[i] = (uint32_t)(i * 2654435761u);
  JpegEnc enc;
  std::vector<uint8_t> out;
  for (int k = 0; k < 5; k++) enc.koduj(F.data(), 640, 640, 480, 80, out, 2);
  const int hlavni = niceVlakna(std::to_string((long)gettid()));
  int pomocnych = 0, s4 = 0;
  if (DIR *d = opendir("/proc/self/task")) {
    while (dirent *e = readdir(d)) {
      if (e->d_name[0] == '.' || std::to_string((long)gettid()) == e->d_name) continue;
      pomocnych++;
      if (niceVlakna(e->d_name) == -4) s4++;
    }
    closedir(d);
  }
  const bool root = geteuid() == 0;
  std::printf("hlavni vlakno nice %d, pomocnych vlaken %d, z toho s nice -4: %d (root %s)\n", hlavni, pomocnych, s4, root ? "ano" : "ne");
  const bool ok = pomocnych >= 1 && (root ? s4 == pomocnych : true) && hlavni == 0;
  std::printf("%s pomocna vlakna JPEG (pasy) maji prioritu jako vykreslovani (nice -4), hlavni vlakno beze zmeny\nVYSLEDEK: %s\n", ok ? "OK   " : "CHYBA", ok ? "0 chyb" : "CHYBA");
  return ok ? 0 : 1;
}
