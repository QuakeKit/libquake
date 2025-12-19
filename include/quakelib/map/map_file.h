#pragma once
#include "brush.h"
#include "entities.h"
#include "types.h"
#include <array>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace quakelib::map {
  const int STANDARD_VERSION = 100;
  const int VALVE_VERSION = 220;

  class QMapFile {
  public:
    QMapFile() { worldSpawn = nullptr; };

    void Parse(const std::string &filename);
    void Parse(const char *buffer);
    void Parse(std::istream &strstr);

    const std::string &VersionString() { return mapVersionStr; };

    int Version() { return mapVersion; };

  private:
    void parse_entity_attributes(std::string l, Entity *ent);
    void parse_entity_planes(std::stringstream &lines, SolidMapEntity *ent);
    void parse_wad_string(const std::string &wads);

  private:
    size_t getOrAddTexture(const std::string &texture);

    int mapVersion = STANDARD_VERSION;
    std::string mapVersionStr = "100";
    SolidMapEntity *worldSpawn;
    std::vector<SolidEntityPtr> solidEntities;
    std::vector<PointEntityPtr> pointEntities;
    std::vector<std::string> textures;
    std::vector<std::string> wads;
    friend class QMap;
  };
} // namespace quakelib::map
