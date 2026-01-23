#include <quakelib/map/map.h>

#include <iostream>

namespace quakelib::map {
  void QMap::LoadBuffer(const char *buffer, getTextureBoundsCb getTextureBounds) {
    map_file = std::make_shared<QMapFile>();
    map_file->Parse(buffer);
    if (getTextureBounds != nullptr) {
      for (int i = 0; i < map_file->textures.size(); i++) {
        textureIDBounds[i] = getTextureBounds(map_file->textures[i].c_str());
      }
    }
    GenerateGeometry(true);
  }

  void QMap::LoadFile(const std::string &filename, getTextureBoundsCb getTextureBounds) {
    map_file = std::make_shared<QMapFile>();
    map_file->Parse(filename);
    if (getTextureBounds != nullptr) {
      for (int i = 0; i < map_file->textures.size(); i++) {
        textureIDBounds[i] = getTextureBounds(map_file->textures[i].c_str());
      }
    }
    GenerateGeometry(true);
  }

  void QMap::GenerateGeometry(bool clipBrushes) {
    for (const auto &se : map_file->solidEntities) {
      se->generateMesh(textureIDTypes, textureIDBounds);
      if (clipBrushes) {
        se->csgUnion();
      }
    }
  }

  void QMap::SetFaceTypeByTextureID(const std::string &texture, MapSurface::eFaceType type) {
    if (map_file == nullptr)
      return;

    for (int i = 0; i < map_file->textures.size(); i++) {
      if (map_file->textures[i].find(texture) != std::string::npos) {
        this->textureIDTypes[i] = type;
        return;
      }
    }
  }

  std::vector<PointEntityPtr> QMap::PointEntitiesByClass(const std::string &className) {
    std::vector<PointEntityPtr> ents;
    for (auto pe : map_file->pointEntities) {
      if (pe->ClassContains(className)) {
        ents.push_back(pe);
      }
    }
    return ents;
  }

  const std::string &QMap::TextureName(int textureID) {
    if (map_file == nullptr || textureID < 0 || textureID >= map_file->textures.size()) {
      static std::string empty = "";
      return empty;
    }
    return map_file->textures[textureID];
  }

  bool QMap::getPolygonsByTextureID(int entityID, int texID, std::vector<FacePtr> &list) {
    if (map_file->solidEntities.size() >= entityID || entityID < 0) {
      return false;
    }

    for (auto &b : map_file->solidEntities[entityID].get()->brushes) {
      for (auto &p : b.Faces()) {
        if (p->textureID == texID) {
          list.push_back(p);
        }
      }
    }
    return !list.empty();
  }

  std::vector<FacePtr> QMap::PolygonsByTexture(int entityID, const std::string &findName) {
    int id = -1;
    for (int i = 0; i < map_file->textures.size(); i++) {
      if (map_file->textures[i] == findName) {
        id = i;
        break;
      }
    }
    std::vector<FacePtr> polyList;
    if (id == -1) {
      return polyList;
    }

    getPolygonsByTextureID(entityID, id, polyList);
    return polyList;
  }

  void QMap::GatherPolygons(int entityID, const polygonGatherCb &cb) {
    if (map_file->solidEntities.size() >= entityID || entityID < 0) {
      return;
    }

    for (int i = 0; i < map_file->textures.size(); i++) {
      std::vector<FacePtr> polyList;
      if (getPolygonsByTextureID(entityID, i, polyList)) {
        cb(polyList, i);
      }
    }
  }
} // namespace quakelib::map
