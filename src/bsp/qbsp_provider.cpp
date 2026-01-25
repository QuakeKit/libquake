#include <algorithm> // for transform
#include <iostream>
#include <quakelib/bsp/lightmap.h>
#include <quakelib/bsp/qbsp_provider.h>
#include <quakelib/wad/palette.h>

namespace quakelib {

  QBspProvider::QBspProvider() : m_bsp(std::make_unique<quakelib::bsp::QBsp>()) {}

  bool QBspProvider::Load(const std::string &path) { return Load(path, quakelib::bsp::QBspConfig()); }

  bool QBspProvider::Load(const std::string &path, const quakelib::bsp::QBspConfig &cfg) {
    m_bsp = std::make_unique<quakelib::bsp::QBsp>(cfg);
    return m_bsp->LoadFile(path.c_str()) == quakelib::bsp::QBSP_OK;
  }

  void QBspProvider::GenerateGeometry(bool csg) {
    // BSP geometry is already generated
  }

  void QBspProvider::SetFaceType(const std::string &textureName, SurfaceType type) {
    m_faceTypes[textureName] = type;
    std::string lower = textureName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower != textureName) {
      m_faceTypes[lower] = type;
    }
  }

  std::vector<SolidEntityPtr> QBspProvider::GetSolidEntities() const {
    std::vector<SolidEntityPtr> res;
    for (const auto &e : m_bsp->SolidEntities()) {
      res.push_back(e);
    }
    return res;
  }

  std::vector<PointEntityPtr> QBspProvider::GetPointEntities() const {
    std::vector<PointEntityPtr> res;
    for (const auto &e : m_bsp->PointEntities()) {
      auto pe = std::dynamic_pointer_cast<PointEntity>(e);
      if (pe)
        res.push_back(pe);
    }
    return res;
  }

  std::vector<std::string> QBspProvider::GetTextureNames() const {
    std::vector<std::string> names;
    for (const auto &t : m_bsp->Textures()) {
      names.push_back(t.name);
    }
    return names;
  }

  std::vector<RenderMesh> QBspProvider::GetEntityMeshes(const SolidEntityPtr &entity) {
    auto bspEnt = std::dynamic_pointer_cast<bsp::SolidEntity>(entity);
    if (!bspEnt)
      return {};

    std::map<std::string, std::vector<std::shared_ptr<bsp::Surface>>> facesByName;

    for (const auto &face : bspEnt->Faces()) {
      std::string name = "";
      if (face->textureReference)
        name = face->textureReference->name;

      // Removed hardcoded "continue" for clip/skip. Now controlled by configuration.

      facesByName[name].push_back(face);
    }

    std::vector<RenderMesh> result;
    for (const auto &[name, faces] : facesByName) {
      RenderMesh mesh;
      mesh.textureName = name;

      if (m_faceTypes.count(name)) {
        mesh.type = m_faceTypes.at(name);
      } else {
        // Check lower case mapping if needed
        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (m_faceTypes.count(lower))
          mesh.type = m_faceTypes.at(lower);
      }

      if (!faces.empty() && faces[0]->textureReference) {
        mesh.textureWidth = faces[0]->textureReference->width;
        mesh.textureHeight = faces[0]->textureReference->height;
      }

      uint32_t currentVertexOffset = 0;
      for (const auto &face : faces) {
        for (const auto &v : face->verts) {
          Vertex qv;
          qv.point = {v.point.x, v.point.y, v.point.z};
          qv.normal = {v.normal.x, v.normal.y, v.normal.z};
          qv.uv = {v.uv.x, v.uv.y};
          qv.lightmap_uv = {v.lm_uv.x, v.lm_uv.y};
          qv.tangent = {0, 0, 0, 0};

          mesh.vertices.push_back(qv);
        }

        for (auto idx : face->indices) {
          mesh.indices.push_back(idx + currentVertexOffset);
        }
        currentVertexOffset += face->verts.size();
      }
      result.push_back(mesh);
    }
    return result;
  }

  std::optional<TextureData> QBspProvider::GetTextureData(const std::string &name) const {
    auto textures = m_bsp->Textures();
    std::string search = name;
    std::transform(search.begin(), search.end(), search.begin(), ::tolower);
    // Remove extension if present, BSP textures names usually don't have it
    // But name passed might have it? Usually not from MapBuilder.

    for (const auto &tex : textures) {
      if (!tex.hasData || tex.name.empty())
        continue;

      std::string tName = tex.name;
      std::transform(tName.begin(), tName.end(), tName.begin(), ::tolower);

      if (tName == search) {
        // Convert to RGBA using default palette
        auto pal = quakelib::wad::Palette::FromBuffer(quakelib::wad::default_palette_lmp, 768);
        TextureData td;
        td.width = tex.width;
        td.height = tex.height;
        td.data.resize(tex.width * tex.height * 4);

        for (int i = 0; i < tex.width * tex.height; ++i) {
          auto c = pal.GetColor(tex.data[i]);
          td.data[i * 4 + 0] = c.rgba[0];
          td.data[i * 4 + 1] = c.rgba[1];
          td.data[i * 4 + 2] = c.rgba[2];
          td.data[i * 4 + 3] = 255;
        }
        return td;
      }
    }
    return std::nullopt;
  }

  std::optional<TextureData> QBspProvider::GetLightmapData() const {
    const auto *lm = m_bsp->LightMap();
    if (!lm)
      return std::nullopt;

    TextureData td;
    td.width = lm->Width();
    td.height = lm->Height();
    const auto &rgba = lm->RGBA();
    td.data.resize(rgba.size() * 4);

    // Copy data
    for (size_t i = 0; i < rgba.size(); ++i) {
      td.data[i * 4 + 0] = rgba[i].rgba[0];
      td.data[i * 4 + 1] = rgba[i].rgba[1];
      td.data[i * 4 + 2] = rgba[i].rgba[2];
      td.data[i * 4 + 3] = rgba[i].rgba[3];
    }
    return td;
  }

} // namespace quakelib
