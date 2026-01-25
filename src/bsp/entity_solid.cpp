#include <memory>
#include <quakelib/bsp/entity_solid.h>

namespace quakelib::bsp {
  SolidEntity::SolidEntity(const bspFileContent &ctx, ParsedEntity *pe) {
    FillFromParsed(pe);

    std::string modelStr = AttributeStr("model");
    if (!modelStr.empty() && modelStr[0] == '*') {
      m_modelId = std::stoi(modelStr.substr(1));
    } else {
      m_modelId = 0;
    }

    auto &m = ctx.models[m_modelId];
    for (int fid = m.face_id; fid < m.face_id + m.face_num; fid++) {
      auto mface = std::make_shared<Surface>();
      mface->Build(ctx, &ctx.faces[fid]);
      m_faces.push_back(mface);
    }
  }

  void SolidEntity::convertToOpenGLCoords() {
    for (auto &surf : m_faces) {
      for (auto &v : surf->verts) {
        auto temp = v.point.y;
        v.point.y = v.point.z;
        v.point.z = -temp;
      }
    }
  }

  const std::vector<SurfacePtr> &SolidEntity::Faces() { return m_faces; }

  bool SolidEntity::IsWorldSpawn() { return m_classname == "worldspawn"; };
} // namespace quakelib::bsp