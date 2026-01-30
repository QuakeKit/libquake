#include "metrics.h"
#include "scene.h"
#include <args/args.hxx>
#include <iomanip>
#include <iostream>
#include <quakelib/map/map.h>
#include <vector>

using std::cout, std::endl;
using namespace quakelib;

void printMetrics();

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

  Metrics::instance().startTimer("Total");
  scene.LoadQuakeMap(args::get(map), opts);
  Metrics::instance().finalizeTimer("Total");

  printMetrics();
  scene.Run();

  return 0;
}

void printMetrics() {
  std::cout << std::endl;
  std::cout << " Metric                 │ Time (ms)" << std::endl;
  std::cout << "────────────────────────┼──────────" << std::endl;
  std::cout << " Geometry Generate      │ " << std::setw(8) << Metrics::instance().getMetrics("geo_generate");
  std::cout << std::endl;
  std::cout << " Mesh Convert           │ " << std::setw(8) << Metrics::instance().getMetrics("mesh_convert");
  std::cout << std::endl;
  std::cout << "────────────────────────┼──────────" << std::endl;
  std::cout << " Total                  │ " << std::setw(8) << Metrics::instance().getMetrics("Total");
  std::cout << std::endl;
}