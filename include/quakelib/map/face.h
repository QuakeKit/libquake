#pragma once

#include "qmath.h"
#include "types.h"
#include <memory>
#include <tuple>

#include <quakelib/surface.h>
#include <quakelib/vertex.h>

namespace quakelib::map {
  class MapSurface;
  using FacePtr = std::shared_ptr<MapSurface>;
  using FaceIter = std::vector<FacePtr>::const_iterator;

  class MapSurface : public Surface {
  public:
    enum eFaceClassification {
      FRONT = 0,
      BACK,
      ON_PLANE,
      SPANNING,
    };

    enum eFaceType { SOLID = 0, CLIP, SKIP, NODRAW };

  public:
    MapSurface() = default;

    MapSurface(const std::array<fvec3, 3> &points, int textureID, StandardUV uv, float rotation, float scaleX,
               float scaleY)
        : planePoints(points), standardUV(uv), textureID(textureID), rotation(rotation), scaleX(scaleX),
          scaleY(scaleY) {
      initPlane();
    };

    MapSurface(const std::array<fvec3, 3> &points, int textureID, ValveUV uv, float rotation, float scaleX,
               float scaleY)
        : planePoints(points), valveUV(uv), textureID(textureID), rotation(rotation), scaleX(scaleX),
          scaleY(scaleY), hasValveUV(true) {
      initPlane();
    };

    MapSurface::eFaceClassification Classify(const MapSurface *other);
    MapSurface::eFaceClassification ClassifyPoint(const fvec3 &v);
    void UpdateAB();
    void UpdateNormals();
    [[nodiscard]] FacePtr Copy() const;

    [[nodiscard]] int TextureID() const { return textureID; };

    const fvec3 &GetPlaneNormal() const { return planeNormal; }

    const float &GetPlaneDist() const { return planeDist; }

    eFaceType Type() const { return type; }

    fvec3 center{}, min{}, max{};
    bool operator==(const MapSurface &arg_) const;

  private:
    fvec4 CalcTangent() { return hasValveUV ? calcValveTangent() : calcStandardTangent(); };

    fvec2 CalcUV(fvec3 vertex, float texW, float texH) {
      return hasValveUV ? calcValveUV(vertex, texW, texH) : calcStandardUV(vertex, texW, texH);
    };

    void initPlane();
    fvec4 calcStandardTangent();
    fvec4 calcValveTangent();
    fvec2 calcStandardUV(fvec3 vertex, float texW, float texH);
    fvec2 calcValveUV(fvec3 vertex, float texW, float texH);
    bool getIntersection(const fvec3 &start, const fvec3 &end, fvec3 &out_intersectionPt,
                         float &out_percentage);
    std::pair<FacePtr, FacePtr> splitFace(const MapSurface *other);

    std::array<fvec3, 3> planePoints{};
    fvec3 planeNormal{};
    float planeDist{};
    StandardUV standardUV{};
    ValveUV valveUV{};
    int textureID{};
    float rotation{};
    float scaleX{};
    float scaleY{};
    eFaceType type = SOLID;
    bool hasValveUV{};

    friend class Brush;
    friend class QMap;
  };
} // namespace quakelib::map
