#include <quakelib/map/map.h>

#include <iostream>

namespace quakelib::map {
  void QMap::LoadBuffer(const char *buffer, getTextureBoundsCb getTextureBounds) {
    m_map_file = std::make_shared<QMapFile>();
    m_map_file->Parse(buffer);
    if (getTextureBounds != nullptr) {
      for (int i = 0; i < m_map_file->m_textures.size(); i++) {
        m_textureIDBounds[i] = getTextureBounds(m_map_file->m_textures[i].c_str());
      }
    }
    GenerateGeometry(true);
  }

  void QMap::LoadFile(const std::string &filename, getTextureBoundsCb getTextureBounds) {
    m_map_file = std::make_shared<QMapFile>();
    m_map_file->Parse(filename);
    if (getTextureBounds != nullptr) {
      for (int i = 0; i < m_map_file->m_textures.size(); i++) {
        m_textureIDBounds[i] = getTextureBounds(m_map_file->m_textures[i].c_str());
      }
    }
    GenerateGeometry(true);
  }

  void QMap::GenerateGeometry(bool clipBrushes) {
    for (const auto &se : m_map_file->m_solidEntities) {
      se->generateMesh(m_textureIDTypes, m_textureIDBounds);
      if (clipBrushes) {
        se->csgUnion();
      }
    }
  }

  void QMap::SetFaceTypeByTextureID(const std::string &texture, MapSurface::eFaceType type) {
    if (m_map_file == nullptr)
      return;

    for (int i = 0; i < m_map_file->m_textures.size(); i++) {
      if (m_map_file->m_textures[i].find(texture) != std::string::npos) {
        this->m_textureIDTypes[i] = type;
        return;
      }
    }
  }

  std::vector<PointEntityPtr> QMap::PointEntitiesByClass(const std::string &className) {
    std::vector<PointEntityPtr> ents;
    for (auto pe : m_map_file->m_pointEntities) {
      if (pe->ClassContains(className)) {
        ents.push_back(pe);
      }
    }
    return ents;
  }

  const std::string &QMap::TextureName(int textureID) {
    if (m_map_file == nullptr || textureID < 0 || textureID >= m_map_file->m_textures.size()) {
      static std::string empty = "";
      return empty;
    }
    return m_map_file->m_textures[textureID];
  }

  bool QMap::getPolygonsByTextureID(int entityID, int texID, std::vector<FacePtr> &list) {
    if (m_map_file->m_solidEntities.size() >= entityID || entityID < 0) {
      return false;
    }

    for (auto &b : m_map_file->m_solidEntities[entityID].get()->m_brushes) {
      for (auto &p : b.Faces()) {
        if (p->TextureID() == texID) {
          list.push_back(p);
        }
      }
    }
    return !list.empty();
  }

  std::vector<FacePtr> QMap::PolygonsByTexture(int entityID, const std::string &findName) {
    int id = -1;
    for (int i = 0; i < m_map_file->m_textures.size(); i++) {
      if (m_map_file->m_textures[i] == findName) {
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
    if (m_map_file->m_solidEntities.size() >= entityID || entityID < 0) {
      return;
    }

    for (int i = 0; i < m_map_file->m_textures.size(); i++) {
      std::vector<FacePtr> polyList;
      if (getPolygonsByTextureID(entityID, i, polyList)) {
        cb(polyList, i);
      }
    }
  }
} // namespace quakelib::map
