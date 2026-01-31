#pragma once

#include "types.h"
#include <memory>
#include <quakelib/qmath.h>
#include <tuple>

#include <quakelib/surface.h>
#include <quakelib/vertex.h>

namespace quakelib::map {
  class MapSurface;
  /**
   * @brief Shared pointer to a MapSurface (face).
   */
  using FacePtr = std::shared_ptr<MapSurface>;
  /**
   * @brief Iterator type for vectors of FacePtr.
   */
  using FaceIter = std::vector<FacePtr>::const_iterator;

  /**
   * @brief Represents a face of a brush in the map editor.
   *
   * Extends the basic Surface class with mapping-specific data like plane equations,
   * texture projection info (Standard/Valve UVs), and content flags.
   */
  class MapSurface : public Surface {
  public:
    /**
     * @brief Classification of a face or point relative to a plane.
     */
    enum eFaceClassification {
      FRONT = 0, ///< The geometry is in front of the plane.
      BACK,      ///< The geometry is behind the plane.
      ON_PLANE,  ///< The geometry lies exactly on the plane.
      SPANNING,  ///< The geometry spans both sides of the plane.
    };

    /**
     * @brief High-level type of the face based on its texture/content.
     */
    enum eFaceType { SOLID = 0, CLIP, SKIP, NODRAW };

  public:
    /**
     * @brief Default constructor.
     */
    MapSurface() = default;

    /**
     * @brief Constructs a MapSurface with Standard UV mapping.
     * @param points Three points defining the plane.
     * @param textureID ID of the texture relative to the map.
     * @param uv Standard UV projection data.
     * @param rotation Texture rotation in degrees.
     * @param scaleX Texture scale on X axis.
     * @param scaleY Texture scale on Y axis.
     */
    MapSurface(const std::array<Vec3, 3> &points, int textureID, StandardUV uv, float rotation, float scaleX,
               float scaleY)
        : m_planePoints(points), m_standardUV(uv), m_textureID(textureID), m_rotation(rotation),
          m_scaleX(scaleX), m_scaleY(scaleY) {
      initPlane();
    };

    /**
     * @brief Constructs a MapSurface with Valve UV mapping (Quake 2/Source).
     * @param points Three points defining the plane.
     * @param textureID ID of the texture.
     * @param uv Valve UV projection data (U and V axes).
     * @param rotation Texture rotation.
     * @param scaleX Texture scale X.
     * @param scaleY Texture scale Y.
     */
    MapSurface(const std::array<Vec3, 3> &points, int textureID, ValveUV uv, float rotation, float scaleX,
               float scaleY)
        : m_planePoints(points), m_valveUV(uv), m_textureID(textureID), m_rotation(rotation),
          m_scaleX(scaleX), m_scaleY(scaleY), m_hasValveUV(true) {
      initPlane();
    };

    /**
     * @brief Classifies another face relative to this face's plane.
     * @param other The other face to test.
     * @return The classification (FRONT, BACK, etc.).
     */
    MapSurface::eFaceClassification Classify(const MapSurface *other);

    /**
     * @brief Classifies a point relative to this face's plane.
     * @param v The point to test.
     * @return The classification.
     */
    MapSurface::eFaceClassification ClassifyPoint(const Vec3 &v);

    /**
     * @brief Recalculates AABB (Axis Aligned Bounding Box).
     */
    void UpdateAB();

    /**
     * @brief Recalculates vertex normals.
     */
    void UpdateNormals();

    /**
     * @brief Creates a deep copy of this face.
     * @return A shared pointer to the new copy.
     */
    [[nodiscard]] FacePtr Copy() const;

    /**
     * @brief Gets the texture ID.
     * @return The texture ID.
     */
    [[nodiscard]] int TextureID() const { return m_textureID; };

    /**
     * @brief Gets the normal vector of the face's plane.
     * @return The plane normal.
     */
    const Vec3 &GetPlaneNormal() const { return m_planeNormal; }

    /**
     * @brief Gets the distance of the plane from the origin.
     * @return The plane distance constant (d in ax+by+cz=d or similar).
     */
    const float &GetPlaneDist() const { return m_planeDist; }

    /**
     * @brief Calculates lightmap UV coordinates for a given vertex on this face.
     * @param vertex The vertex position.
     * @return The calculated UV coordinates.
     */
    Vec2 CalcLightmapUV(Vec3 vertex) {
      return m_hasValveUV ? calcValveLightmapUV(vertex) : calcStandardLightmapUV(vertex);
    };

    /**
     * @brief Calculates world position from lightmap UV coordinates.
     * @param uv The lightmap UV coordinates.
     * @return The corresponding world position.
     */
    Vec3 CalcWorldPosFromLightmapUV(Vec2 uv) {
      return m_hasValveUV ? calcWorldFromValveLightmapUV(uv) : calcWorldFromStandardLightmapUV(uv);
    }

    /**
     * @brief Gets the face type (e.g. SOLID, CLIP).
     * @return The face type.
     */
    eFaceType Type() const { return m_type; }

    Vec3 center{}, min{}, max{}; ///< Geometric properties: center and AABB bounds.

    bool operator==(const MapSurface &arg_) const;

  private:
    Vec4 CalcTangent() { return m_hasValveUV ? calcValveTangent() : calcStandardTangent(); };

    Vec2 CalcUV(Vec3 vertex, float texW, float texH) {
      return m_hasValveUV ? calcValveUV(vertex, texW, texH) : calcStandardUV(vertex, texW, texH);
    };

    void initPlane();
    Vec4 calcStandardTangent();
    Vec4 calcValveTangent();
    Vec2 calcStandardUV(Vec3 vertex, float texW, float texH);
    Vec2 calcValveUV(Vec3 vertex, float texW, float texH);
    Vec2 calcStandardLightmapUV(Vec3 vertex);
    Vec2 calcValveLightmapUV(Vec3 vertex);
    Vec3 calcWorldFromStandardLightmapUV(Vec2 uv);
    Vec3 calcWorldFromValveLightmapUV(Vec2 uv);
    bool getIntersection(const Vec3 &start, const Vec3 &end, Vec3 &out_intersectionPt, float &out_percentage);
    std::pair<FacePtr, FacePtr> splitFace(const MapSurface *other);

    std::array<Vec3, 3> m_planePoints{};
    Vec3 m_planeNormal{};
    float m_planeDist{};
    StandardUV m_standardUV{};
    ValveUV m_valveUV{};
    int m_textureID{};
    float m_rotation{};
    float m_scaleX{};
    float m_scaleY{};
    eFaceType m_type = SOLID;
    bool m_hasValveUV{};

    friend class Brush;
    friend class QMap;
    friend class SolidMapEntity;
  };
} // namespace quakelib::map
