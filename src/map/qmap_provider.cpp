#include <iostream>
#include <quakelib/map/map.h>
#include <quakelib/map/qmap_provider.h>
#include <xatlas/xatlas.h>

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

  std::vector<SolidEntityPtr> QMapProvider::GetSolidEntities(const std::string &className) const {
    std::vector<SolidEntityPtr> res;
    for (const auto &e : m_map.SolidEntities()) {
      if (e->ClassName() == className) {
        res.push_back(e);
      }
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

  std::vector<PointEntityPtr> QMapProvider::GetPointEntities(const std::string &className) const {
    std::vector<PointEntityPtr> res;
    for (const auto &e : m_map.PointEntities()) {
      if (e->ClassName() == className) {
        res.push_back(e);
      }
    }
    return res;
  }

  std::vector<RenderMesh> QMapProvider::GetEntityMeshes(const SolidEntityPtr &entity) {
    auto mapEnt = std::dynamic_pointer_cast<map::SolidMapEntity>(entity);
    if (!mapEnt)
      return {};

    // Phase 1: Batch faces by texture ID
    std::map<int, std::vector<map::FacePtr>> batchedFaces;
    const auto &brushes = mapEnt->Brushes();

    for (const auto &b : brushes) {
      for (const auto &p : b.Faces()) {
        if (p->Type() == map::MapSurface::CLIP || p->Type() == map::MapSurface::SKIP ||
            p->Type() == map::MapSurface::NODRAW) {
          // We still batch them, but the render mesh will have the type set
        }
        batchedFaces[p->TextureID()].push_back(p);
      }
    }

    // Phase 2: Build meshes with vertex welding
    std::vector<RenderMesh> result;
    auto texNames = m_map.TextureNames();

    for (auto const &[texID, faces] : batchedFaces) {
      if (faces.empty())
        continue;

      RenderMesh mesh;
      if (texID >= 0 && texID < texNames.size())
        mesh.textureName = texNames[texID];

      // Determine type from first face (they share texture ID)
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

      weldVertices(mesh, faces);
      result.push_back(mesh);
    }

    // Phase 3: Generate lightmap UVs for all meshes
    generateLightmapUVs(result);

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

  void QMapProvider::weldVertices(RenderMesh &mesh, const std::vector<quakelib::map::FacePtr> &faces) {
    constexpr float weld_epsilon = 0.001f;
    std::vector<uint32_t> vertexRemap;

    for (const auto &face : faces) {
      const auto &verts = face->Vertices();
      const auto &inds = face->Indices();

      for (const auto &vert : verts) {
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

          // Check normal
          float dnx = existing.normal[0] - vert.normal[0];
          float dny = existing.normal[1] - vert.normal[1];
          float dnz = existing.normal[2] - vert.normal[2];
          if (dnx * dnx + dny * dny + dnz * dnz >= weld_epsilon * weld_epsilon)
            continue;

          // All attributes match - can weld
          existingIndex = i;
          break;
        }

        if (existingIndex != UINT32_MAX) {
          vertexRemap.push_back(existingIndex);
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
  }

  void QMapProvider::generateLightmapUVs(std::vector<RenderMesh> &meshes) {
    if (meshes.empty())
      return;

    xatlas::Atlas *atlas = xatlas::Create();

    // Add all meshes to the atlas
    for (const auto &mesh : meshes) {
      if (mesh.vertices.empty() || mesh.indices.empty())
        continue;

      xatlas::MeshDecl meshDecl;
      meshDecl.vertexCount = mesh.vertices.size();
      meshDecl.vertexPositionData = &mesh.vertices[0].point;
      meshDecl.vertexPositionStride = sizeof(Vertex);
      meshDecl.vertexNormalData = &mesh.vertices[0].normal;
      meshDecl.vertexNormalStride = sizeof(Vertex);
      meshDecl.vertexUvData = &mesh.vertices[0].uv;
      meshDecl.vertexUvStride = sizeof(Vertex);
      meshDecl.indexCount = mesh.indices.size();
      meshDecl.indexData = mesh.indices.data();
      meshDecl.indexFormat = xatlas::IndexFormat::UInt32;

      xatlas::AddMesh(atlas, meshDecl);
    }

    // Generate atlas with all meshes packed together
    xatlas::Generate(atlas);

    // Update each mesh with its portion of the atlas
    for (size_t meshIdx = 0; meshIdx < meshes.size() && meshIdx < atlas->meshCount; meshIdx++) {
      auto &mesh = meshes[meshIdx];
      const xatlas::Mesh &xatlasMesh = atlas->meshes[meshIdx];

      // Rebuild vertices and indices from xatlas output
      std::vector<Vertex> newVertices;
      newVertices.reserve(xatlasMesh.vertexCount);

      for (uint32_t i = 0; i < xatlasMesh.vertexCount; i++) {
        const xatlas::Vertex &xv = xatlasMesh.vertexArray[i];
        const Vertex &origVert = mesh.vertices[xv.xref];

        Vertex newVert = origVert;
        // Update with xatlas-generated lightmap UVs (normalized to 0-1)
        newVert.lightmap_uv[0] = xv.uv[0] / atlas->width;
        newVert.lightmap_uv[1] = xv.uv[1] / atlas->height;

        newVertices.push_back(newVert);
      }

      // Update indices
      std::vector<uint32_t> newIndices;
      newIndices.reserve(xatlasMesh.indexCount);
      for (uint32_t i = 0; i < xatlasMesh.indexCount; i++) {
        newIndices.push_back(xatlasMesh.indexArray[i]);
      }

      // Replace mesh data
      mesh.vertices = std::move(newVertices);
      mesh.indices = std::move(newIndices);
    }

    xatlas::Destroy(atlas);
  }

  void
  QMapProvider::SetTextureBoundsProvider(std::function<std::pair<int, int>(const std::string &)> provider) {
    m_map.RegisterTextureBounds([provider](const char *name) -> map::textureBounds {
      auto p = provider(name);
      return {(float)p.first, (float)p.second};
    });
  }

} // namespace quakelib
