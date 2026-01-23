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
    QMapFile() { m_worldSpawn = nullptr; };

    void Parse(const std::string &filename);
    void Parse(const char *buffer);
    void Parse(std::istream &strstr);

    const std::string &VersionString() { return m_mapVersionStr; };

    int Version() { return m_mapVersion; };

  private:
    void parse_entity_attributes(std::string l, Entity *ent);
    void parse_entity_planes(std::stringstream &lines, SolidMapEntity *ent);
    void parse_wad_string(const std::string &wads);

  private:
    size_t getOrAddTexture(const std::string &texture);

    int m_mapVersion = STANDARD_VERSION;
    std::string m_mapVersionStr = "100";
    SolidMapEntity *m_worldSpawn;
    std::vector<SolidEntityPtr> m_solidEntities;
    std::vector<PointEntityPtr> m_pointEntities;
    std::vector<std::string> m_textures;
    std::vector<std::string> m_wads;
    friend class QMap;
  };
} // namespace quakelib::map
