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
    m_center = CalculateCenterFromBBox(m_min, m_max);
  }

  void SolidMapEntity::csgUnion() {
    if (!m_brushes.empty()) {
      m_min = m_brushes[0].min;
      m_max = m_brushes[0].max;
    }
    for (auto &b1 : m_brushes) {
      auto cpBrush = b1;
      for (auto &b2 : m_brushes) {
        if (&b1 == &b2 || b2.m_faces.empty()) {
          continue;
        }

        if (!b1.DoesIntersect(b2) || (b1.IsBlockVolume() || b2.IsBlockVolume())) {
          continue;
        }

        auto clippedFaces = cpBrush.clipToBrush(b2);
        cpBrush.m_faces = clippedFaces;
      }
      if (!cpBrush.m_faces.empty()) {
        m_clippedBrushes.push_back(cpBrush);
        cpBrush.GetBiggerBBox(m_min, m_max);
        m_stats_clippedFaces += b1.m_faces.size() - cpBrush.m_faces.size();
      }
    }
    m_center = CalculateCenterFromBBox(m_min, m_max);
    if (m_brushes.size() > 0 && m_clippedBrushes.size() > 0) {
      m_wasClipped = true;
    }
  }
} // namespace quakelib::map