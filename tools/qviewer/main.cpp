#include "data.h"
#include <iostream>
#include <quakelib/entity_parser.h>
#include <strstream>

using std::cout, std::endl;
using namespace quakelib;

class MyPoint : public PointEntity {};

int main() {
  EntityParser::ParseEntites(mapbuff, [](ParsedEntity *pe) {
    if (pe->type != EntityType::SOLID && pe->type != EntityType::WORLDSPAWN) {
      return;
    }

    SolidEntity se;
    se.FillFromParsed(pe);
    cout << se.ClassName() << endl;

    for (auto child : pe->children) {
      for (std::string line; std::getline(child->lines, line);) {
        cout << line + "\n";
      }
    }
  });
}