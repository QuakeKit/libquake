#include "data.h"
#include <iostream>
#include <quakelib/map/map.h>
#include <strstream>

using std::cout, std::endl;
using namespace quakelib;

/*
class MyPoint : public PointEntity {};
*/
int main() {
  /*
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
  */

  map::QMap map;
  map.LoadBuffer(mapbuff, nullptr);
  for (auto &solid : map.SolidEntities()) {
    cout << "Solid Entity: " << solid->ClassName() << endl;
    for (auto &brush : solid->Brushes()) {
      cout << " Brush with " << brush.Faces().size() << " faces." << endl;
      for (auto &face : brush.Faces()) {
        cout << "  Face with " << face->Vertices().size() << " vertices." << endl;
        cout << "  Tex " << map.TextureName(face->TextureID()) << endl;
      }
    }
  }

  return 0;
}