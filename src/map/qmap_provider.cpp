#include <iostream>
#include <quakelib/map/map.h>
#include <quakelib/map/qmap_provider.h>

namespace quakelib {

  bool QMapProvider::Load(const std::string &path) {
    try {
      m_map.LoadFile(path, nullptr);

      // Default Common Types
      SetFaceType("clip", SurfaceType::CLIP);
      SetFaceType("trigger", SurfaceType::CLIP); // Triggers are usually clip/nodraw
      SetFaceType("skip", SurfaceType::SKIP);

      return true;
    } catch (const std::exception &e) {
      std::cerr << "Failed to load MAP: " << e.what() << std::endl;
      return false;
    }
  }

  bool QMapProvider::Load(const std::string &path, const quakelib::map::QMapConfig &cfg) {
    m_map = quakelib::map::QMap(cfg);
    return Load(path);
  }

  void QMapProvider::GenerateGeometry(bool csg) { m_map.GenerateGeometry(); }

  void QMapProvider::SetFaceType(const std::string &textureName, SurfaceType type) {
    map::MapSurface::eFaceType mapType = map::MapSurface::SOLID;
    switch (type) {
    case SurfaceType::CLIP:
      mapType = map::MapSurface::CLIP;
      break;
    case SurfaceType::SKIP:
      mapType = map::MapSurface::SKIP;
      break;
    case SurfaceType::NODRAW:
      mapType = map::MapSurface::NODRAW;
      break;
    default:
      break;
    }
    m_map.SetFaceTypeByTextureID(textureName, mapType);
  }

  std::vector<SolidEntityPtr> QMapProvider::GetSolidEntities() const {
    std::vector<SolidEntityPtr> res;
    for (const auto &e : m_map.SolidEntities()) {
      res.push_back(e);
    }
    return res;
  }

  std::vector<PointEntityPtr> QMapProvider::GetPointEntities() const {
    std::vector<PointEntityPtr> res;
    for (const auto &e : m_map.PointEntities()) {
      res.push_back(e);
    }
    return res;
  }

  std::vector<RenderMesh> QMapProvider::GetEntityMeshes(const SolidEntityPtr &entity) {
    auto mapEnt = std::dynamic_pointer_cast<map::SolidMapEntity>(entity);
    if (!mapEnt)
      return {};

    std::map<int, std::vector<map::FacePtr>> batchedFaces;
    const auto &brushes = mapEnt->Brushes();

    for (const auto &b : brushes) {
      for (const auto &p : b.Faces()) {
        if (p->Type() == map::MapSurface::CLIP || p->Type() == map::MapSurface::SKIP ||
            p->Type() == map::MapSurface::NODRAW) {
          // We still batch them, but the render mesh will have the type set
          // Wait, QMap stores type in the face itself.
        }
        batchedFaces[p->TextureID()].push_back(p);
      }
    }

    std::vector<RenderMesh> result;
    auto texNames = m_map.TextureNames();

    for (auto const &[texID, faces] : batchedFaces) {
      RenderMesh mesh;
      if (texID >= 0 && texID < texNames.size())
        mesh.textureName = texNames[texID];

      // Determine type from first face (they used same texture ID so presumably same type)
      if (!faces.empty()) {
        switch (faces[0]->Type()) {
        case map::MapSurface::CLIP:
          mesh.type = SurfaceType::CLIP;
          break;
        case map::MapSurface::SKIP:
          mesh.type = SurfaceType::SKIP;
          break;
        case map::MapSurface::NODRAW:
          mesh.type = SurfaceType::NODRAW;
          break;
        default:
          mesh.type = SurfaceType::SOLID;
          break;
        }
      }

      uint32_t currentVertexOffset = 0;
      for (const auto &face : faces) {
        const auto &verts = face->Vertices();
        const auto &inds = face->Indices();

        mesh.vertices.insert(mesh.vertices.end(), verts.begin(), verts.end());

        for (auto idx : inds) {
          mesh.indices.push_back(idx + currentVertexOffset);
        }
        currentVertexOffset += verts.size();
      }
      result.push_back(mesh);
    }
    return result;
  }

  std::vector<std::string> QMapProvider::GetTextureNames() const { return m_map.TextureNames(); }

  std::vector<std::string> QMapProvider::GetRequiredWads() const {
    std::vector<std::string> wads;
    if (m_map.HasWads()) {
      for (const auto &w : m_map.Wads())
        wads.push_back(w);
    }
    return wads;
  }

  void
  QMapProvider::SetTextureBoundsProvider(std::function<std::pair<int, int>(const std::string &)> provider) {
    m_map.RegisterTextureBounds([provider](const char *name) -> map::textureBounds {
      auto p = provider(name);
      return {(float)p.first, (float)p.second};
    });
  }

} // namespace quakelib
