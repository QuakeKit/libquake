#include <quakelib/map/entities.h>

namespace quakelib::map {

  void SolidMapEntity::generateMesh(const std::map<int, MapSurface::eFaceType> &faceTypes,
                                    const std::map<int, textureBounds> &texBounds) {
    if (!m_brushes.empty()) {
      m_min = m_brushes[0].min;
      m_max = m_brushes[0].max;
    }
    for (auto &b : m_brushes) {
      b.buildGeometry(faceTypes, texBounds);
      b.GetBiggerBBox(m_min, m_max);
    }
    m_center = math::CalculateCenterFromBBox(m_min, m_max);
  }

  void SolidMapEntity::convertToOpenGLCoords() {
    auto convertBrush = [](Brush &b) {
      for (auto &f : b.Faces()) {
        for (auto &v : f->VerticesRW()) {
          auto temp = v.point[1];
          v.point[1] = v.point[2];
          v.point[2] = -temp;

          auto tempN = v.normal[1];
          v.normal[1] = v.normal[2];
          v.normal[2] = -tempN;
        }
      }
    };
    for (auto &b : m_brushes) {
      convertBrush(b);
    }
    for (auto &b : m_clippedBrushes) {
      convertBrush(b);
    }

    auto swapYz = [](Vec3 &v) {
      auto temp = v[1];
      v[1] = v[2];
      v[2] = -temp;
    };
    swapYz(m_center);
    swapYz(m_min);
    swapYz(m_max);

    if (m_min[2] > m_max[2]) {
      std::swap(m_min[2], m_max[2]);
    }
  }

} // namespace quakelib::map