#include <quakelib/entity_parser.h>
#include <quakelib/map/map_file.h>

#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

namespace quakelib::map {
  static std::vector<std::string> rexec_vec(std::string line, const std::string &regexstr) {
    std::stringstream results;
    std::regex re(regexstr, std::regex::icase);

    std::vector<std::string> matches;
    const std::sregex_token_iterator end;
    for (std::sregex_token_iterator it(line.begin(), line.end(), re, 1); it != end; it++) {
      matches.push_back(*it);
    }

    return matches;
  }

  inline std::string &ltrim(std::string &s, const char *t = " ") {
    s.erase(0, s.find_first_not_of(t));
    return s;
  }

  void QMapFile::Parse(char const *buff) {
    auto strstr = std::stringstream(buff);
    Parse(strstr);
  }

  void QMapFile::Parse(const std::string &filename) {
    auto strstr = std::fstream(filename);
    Parse(strstr);
    strstr.close();
  }

  void QMapFile::Parse(std::istream &stream) {
    EntityParser::ParseEntites(stream, [&](ParsedEntity *pe) {
      switch (pe->type) {
      case EntityType::POINT: {
        auto ent = std::make_shared<PointEntity>();
        ent->FillFromParsed(pe);
        this->pointEntities.push_back(ent);
        break;
      }
      case EntityType::SOLID:
      case EntityType::WORLDSPAWN: {
        auto sent = new SolidMapEntity();
        sent->FillFromParsed(pe);
        this->solidEntities.push_back(SolidEntityPtr(sent));
        if (pe->type == EntityType::WORLDSPAWN) {
          this->worldSpawn = sent;
        }
        for (auto &child : pe->children) {
          std::stringstream lines;
          for (std::string line; std::getline(child->lines, line);) {
            lines << line << std::endl;
          }
          parse_entity_planes(lines, sent);
        }
        break;
      }
      default:
        break;
      }
    });
  }

  void QMapFile::parse_entity_planes(std::stringstream &lines, SolidMapEntity *ent) {
    Brush brush;
    for (std::string line; std::getline(lines, line);) {
      std::string chars = "()[]";
      for (unsigned int i = 0; i < chars.length(); ++i) {
        line.erase(std::remove(line.begin(), line.end(), chars[i]), line.end());
      }

      std::stringstream l(line);
      std::array<fvec3, 3> facePoints{};
      l >> facePoints[0][0] >> facePoints[0][1] >> facePoints[0][2];
      l >> facePoints[1][0] >> facePoints[1][1] >> facePoints[1][2];
      l >> facePoints[2][0] >> facePoints[2][1] >> facePoints[2][2];

      std::string texture;
      l >> texture;

      ValveUV valveUV{};
      StandardUV standardUV{};
      switch (mapVersion) {
      case VALVE_VERSION:
        l >> valveUV.u[0] >> valveUV.u[1] >> valveUV.u[2] >> valveUV.u[3];
        l >> valveUV.v[0] >> valveUV.v[1] >> valveUV.v[2] >> valveUV.v[3];
        break;
      default:
        l >> standardUV.u >> standardUV.v;
        break;
      }

      float rotation{}, scaleX{}, scaleY{};
      l >> rotation >> scaleX >> scaleY;

      auto face = mapVersion == STANDARD_VERSION
                      ? std::make_shared<MapSurface>(facePoints, getOrAddTexture(texture), standardUV,
                                                     rotation, scaleX, scaleY)
                      : std::make_shared<MapSurface>(facePoints, getOrAddTexture(texture), valveUV, rotation,
                                                     scaleX, scaleY);

      brush.faces.push_back(face);
    }
    ent->brushes.push_back(brush);
  }

  size_t QMapFile::getOrAddTexture(const std::string &texture) {
    for (int i = 0; i < textures.size(); i++) {
      if (textures[i] == texture)
        return i;
    }
    textures.push_back(texture);
    size_t ret = textures.size() - 1;
    return ret;
  }
} // namespace quakelib::map
