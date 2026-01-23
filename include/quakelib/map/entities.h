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
    const std::vector<Brush> &GetOriginalBrushes() { return brushes; }

    const std::vector<Brush> &Brushes() {
      if (wasClipped) {
        return clippedBrushes;
      }
      return brushes;
    }

    const std::vector<Brush> &GetClippedBrushes() const { return clippedBrushes; }

    const fvec3 &GetCenter() const { return center; }

    const fvec3 &GetMin() const { return min; }

    const fvec3 &GetMax() const { return max; }

    // stats getter
    size_t StatsClippedFaces() const { return stats_clippedFaces; }

  private:
    void generateMesh(const std::map<int, MapSurface::eFaceType> &faceTypes,
                      const std::map<int, textureBounds> &texBounds);
    void csgUnion();

    std::vector<Brush> brushes;
    std::vector<Brush> clippedBrushes;
    bool hasPhongShading{};
    std::vector<int> textureIDs;
    size_t stats_clippedFaces{};
    bool wasClipped = false;

    fvec3 center{0};
    fvec3 min{0};
    fvec3 max{0};

    friend class QMapFile;
    friend class QMap;
  };

  using BaseEntityPtr = std::shared_ptr<Entity>;
  using SolidEntityPtr = std::shared_ptr<SolidMapEntity>;
  using PointEntityPtr = std::shared_ptr<PointEntity>;
} // namespace quakelib::map
