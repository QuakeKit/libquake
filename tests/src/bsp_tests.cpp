#include "../inc/bsp_dummy.h"
#include <quakelib/entity_parser.h>
#include <snitch/snitch.hpp>

using namespace quakelib;

const int POINT_COUNT = 3;
const int SOLID_COUNT = 5;
const int WORLDSPAWN_COUNT = 1;

TEST_CASE("parse bsp entities", "[bsp/entities]") {
  int solid_hits = 0;
  int point_hits = 0;
  int worldspawn_hits = 0;

  EntityParser::ParseEntites(entbuff, [&](ParsedEntity *pe) {
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