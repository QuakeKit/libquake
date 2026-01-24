#include "scene.h"
#include <iostream>
#include <quakelib/map/map.h>
#include <strstream>

using std::cout, std::endl;
using namespace quakelib;

void banner() {
  cout << "QFORMAT test app\n"
       << "----------------" << endl;
}

int main(int argc, char *argv[]) {
  banner();
  
  std::string mapPath = "/Users/tinogohlert/workspace/chunkycat/quake_data/maps/orig/START.MAP";
  std::string wadPath = "/Users/tinogohlert/workspace/chunkycat/quake_data/wads/";

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-m" && i + 1 < argc) {
      mapPath = argv[++i];
    } else if (arg == "-w" && i + 1 < argc) {
      wadPath = argv[++i];
    }
  }

  auto scene = Scene();

  QuakeMapOptions opts;
  opts.wadPath = wadPath;
  opts.backgroundColor = BLACK;

  scene.LoadQuakeMap(mapPath, opts);
  scene.Run();
  return 0;
}