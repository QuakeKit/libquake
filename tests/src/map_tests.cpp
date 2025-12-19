#include "../inc/map_dummy.h"
#include <quakelib/entity_parser.h>
#include <quakelib/map/map.h>
#include <snitch/snitch.hpp>

using namespace quakelib;

const int POINT_COUNT = 3;
const int SOLID_COUNT = 1;
const int WORLDSPAWN_COUNT = 1;

TEST_CASE("parse map entities", "[map/entities]") {
  int solid_hits = 0;
  int point_hits = 0;
  int worldspawn_hits = 0;

  EntityParser::ParseEntites(mapbuff, [&](ParsedEntity *pe) {
    Entity *ent = nullptr;
    if (pe->type == EntityType::POINT) {
      ent = new PointEntity();
      point_hits++;
    }
    if (pe->type == EntityType::SOLID) {
      ent = new SolidEntity();
      solid_hits++;
    }
    if (pe->type == EntityType::WORLDSPAWN) {
      ent = new WorldSpawnEntity();
      worldspawn_hits++;
    }
    if (ent != nullptr) {
      ent->FillFromParsed(pe);
      if (pe->type == EntityType::SOLID && ent->ClassName() == "func_door") {
        REQUIRE(ent->AttributeStr("angle") != "");
        REQUIRE(ent->AttributeStr("wait") != "");
        REQUIRE(ent->AttributeStr("speed") != "");
      }
      if (pe->type == EntityType::POINT && ent->ClassName() == "light") {
        REQUIRE(ent->AttributeFloat("light") == 150);
      }
    }
  });

  REQUIRE(point_hits == POINT_COUNT);
  REQUIRE(solid_hits == SOLID_COUNT);
  REQUIRE(worldspawn_hits == WORLDSPAWN_COUNT);
}

TEST_CASE("parse map", "[map/parsing]") {
  auto m = new map::QMap();
  m->LoadBuffer(mapbuff, [&](const char *textureName) {
    // we don't check texture name catching yet
    return map::textureBounds{0, 0};
  });
  REQUIRE(m->GetPointEntities().size() == POINT_COUNT);
}