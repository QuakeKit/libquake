//
// Created by tgoeh on 10.11.2023.
//
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "brush.h"
#include "entities.h"
#include "map_file.h"
#include "types.h"

namespace quakelib::map {
  using polygonGatherCb = std::function<void(std::vector<FacePtr>, int)>;
  using getTextureBoundsCb = std::function<textureBounds(const char *textureName)>;

  class QMap {
  public:
    QMap() = default;
    ~QMap() = default;

    void LoadFile(const std::string &filename, getTextureBoundsCb getTextureBounds = nullptr);
    void LoadBuffer(const char *buffer, getTextureBoundsCb getTextureBounds);
    void GenerateGeometry(bool clipBrushes = true);
    void GatherPolygons(int entityID, const polygonGatherCb &);

    std::vector<FacePtr> PolygonsByTexture(int entityID, const std::string &texName);

    const std::vector<std::string> &Wads() { return map_file->wads; };

    bool HasWads() { return !map_file->wads.empty(); };

    const std::vector<std::string> &TextureNames() { return map_file->textures; };

    const std::string &TextureName(int textureID);

    QMapFile *MapData() { return map_file.get(); };

    void SetFaceTypeByTextureID(const std::string &texture, MapSurface::eFaceType type);

    const SolidMapEntity *WorldSpawn() { return map_file->worldSpawn; }

    const std::vector<SolidEntityPtr> &SolidEntities() { return map_file->solidEntities; };

    const std::vector<PointEntityPtr> &PointEntities() { return map_file->pointEntities; };

    std::vector<PointEntityPtr> PointEntitiesByClass(const std::string &className);

  private:
    bool getPolygonsByTextureID(int entityID, int texID, std::vector<FacePtr> &list);

    std::map<int, MapSurface::eFaceType> textureIDTypes;
    std::map<int, textureBounds> textureIDBounds;
    std::shared_ptr<QMapFile> map_file;
  };
} // namespace quakelib::map
