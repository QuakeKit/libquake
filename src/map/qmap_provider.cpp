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

      // Weld vertices while batching faces
      // Note: We must check all vertex attributes including lightmap UVs because
      // vertices at edges/corners may have the same position but different lightmap UVs
      // (each face gets its own region in the lightmap atlas)
      constexpr float weld_epsilon = 0.001f;
      std::vector<uint32_t> vertexRemap;

      uint32_t totalVerts = 0;
      uint32_t weldedVerts = 0;

      for (const auto &face : faces) {
        const auto &verts = face->Vertices();
        const auto &inds = face->Indices();

        for (const auto &vert : verts) {
          totalVerts++;
          continue;
          // Check if this vertex already exists in the mesh
          // Must match position, UVs, normal, and tangent
          uint32_t existingIndex = UINT32_MAX;
          for (uint32_t i = 0; i < mesh.vertices.size(); ++i) {
            const auto &existing = mesh.vertices[i];

            // Check position
            float dx = existing.point[0] - vert.point[0];
            float dy = existing.point[1] - vert.point[1];
            float dz = existing.point[2] - vert.point[2];
            float distSq = dx * dx + dy * dy + dz * dz;

            if (distSq >= weld_epsilon * weld_epsilon)
              continue;

            // Check texture UVs
            float du = existing.uv[0] - vert.uv[0];
            float dv = existing.uv[1] - vert.uv[1];
            if (du * du + dv * dv >= weld_epsilon * weld_epsilon)
              continue;

            // Check lightmap UVs - must match to weld
            // Vertices at edges have different lightmap UVs for different faces
            // because each face gets its own region in the lightmap atlas
            float dlu = existing.lightmap_uv[0] - vert.lightmap_uv[0];
            float dlv = existing.lightmap_uv[1] - vert.lightmap_uv[1];
            if (dlu * dlu + dlv * dlv >= weld_epsilon * weld_epsilon)
              continue;

            // Check normal
            float dnx = existing.normal[0] - vert.normal[0];
            float dny = existing.normal[1] - vert.normal[1];
            float dnz = existing.normal[2] - vert.normal[2];
            if (dnx * dnx + dny * dny + dnz * dnz >= weld_epsilon * weld_epsilon)
              continue;

            // All attributes match
            existingIndex = i;
            break;
          }

          if (existingIndex != UINT32_MAX) {
            vertexRemap.push_back(existingIndex);
            weldedVerts++;
          } else {
            vertexRemap.push_back(mesh.vertices.size());
            mesh.vertices.push_back(vert);
          }
        }

        for (auto idx : inds) {
          mesh.indices.push_back(vertexRemap[idx]);
        }

        vertexRemap.clear();
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
