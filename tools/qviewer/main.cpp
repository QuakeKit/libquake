#include "scene.h"
#include <args/args.hxx>
#include <iostream>
#include <quakelib/map/map.h>
#include <strstream>

using std::cout, std::endl;
using namespace quakelib;

void banner() {
  cout << R"(
   ██████╗ ██╗   ██╗██╗███████╗██╗    ██╗███████╗██████╗ 
  ██╔═══██╗██║   ██║██║██╔════╝██║    ██║██╔════╝██╔══██╗
  ██║   ██║██║   ██║██║█████╗  ██║ █╗ ██║█████╗  ██████╔╝
  ██║▄▄ ██║╚██╗ ██╔╝██║██╔══╝  ██║███╗██║██╔══╝  ██╔══██╗
  ╚██████╔╝ ╚████╔╝ ██║███████╗╚███╔███╔╝███████╗██║  ██║
   ╚══▀▀═╝   ╚═══╝  ╚═╝╚══════╝ ╚══╝╚══╝ ╚══════╝╚═╝  ╚═╝
                Quake 1 Map and BSP Viewer
  -------------------------------------------------------
)";
}

int main(int argc, char *argv[]) {
  banner();

  args::ArgumentParser parser("qviewer - view quake1 map and bsp files", "");
  args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
  args::ValueFlag<std::string> wads(parser, "wads", "path to wads", {'w'});
  args::ValueFlag<std::string> map(parser, "map", "path to map or bsp", {'m'});

  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Help) {
    std::cout << parser;
    return 0;
  } catch (args::ParseError e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  } catch (args::ValidationError e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }

  if (!map || !wads) {
    std::cout << parser;
    return 0;
  }

  auto scene = Scene();

  QuakeMapOptions opts;
  opts.wadPath = args::get(wads);
  opts.backgroundColor = BLACK;

  scene.LoadQuakeMap(args::get(map), opts);
  scene.Run();
  return 0;
}