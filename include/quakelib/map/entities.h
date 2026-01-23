#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "brush.h"
#include "types.h"

#include <quakelib/entities.h>

namespace quakelib::map {
  class SolidMapEntity : public SolidEntity {
  public:
    const std::vector<Brush> &GetOriginalBrushes() { return m_brushes; }

    const std::vector<Brush> &Brushes() {
      if (m_wasClipped) {
        return m_clippedBrushes;
      }
      return m_brushes;
    }

    const std::vector<Brush> &GetClippedBrushes() const { return m_clippedBrushes; }

    const fvec3 &GetCenter() const { return m_center; }

    const fvec3 &GetMin() const { return m_min; }

    const fvec3 &GetMax() const { return m_max; }

    // stats getter
    size_t StatsClippedFaces() const { return m_stats_clippedFaces; }

  private:
    void generateMesh(const std::map<int, MapSurface::eFaceType> &faceTypes,
                      const std::map<int, textureBounds> &texBounds);
    void csgUnion();

    std::vector<Brush> m_brushes;
    std::vector<Brush> m_clippedBrushes;
    bool m_hasPhongShading{};
    std::vector<int> m_textureIDs;
    size_t m_stats_clippedFaces{};
    bool m_wasClipped = false;

    fvec3 m_center{0};
    fvec3 m_min{0};
    fvec3 m_max{0};

    friend class QMapFile;
    friend class QMap;
  };

  using BaseEntityPtr = std::shared_ptr<Entity>;
  using SolidEntityPtr = std::shared_ptr<SolidMapEntity>;
  using PointEntityPtr = std::shared_ptr<PointEntity>;
} // namespace quakelib::map
