#pragma once

#include <quakelib/qmath.h>
#include <vector>

namespace quakelib {

  using math::Vec2;
  using math::Vec3;
  using math::Vec4;

  /**
   * @brief Represents a vertex in 3D space with associated attributes.
   */
  struct Vertex {
    Vec3 point;       ///< The 3D position of the vertex.
    Vec3 normal;      ///< The normal vector at this vertex.
    Vec2 uv;          ///< Texture coordinates.
    Vec2 lightmap_uv; ///< Lightmap coordinates.
    Vec4 tangent;     ///< Tangent vector (xyz) and bitangent sign (w).

    /**
     * @brief Checks if this vertex's position exists in a given list.
     * @param list The vector of vertices to search.
     * @return True if a vertex with the same position exists in the list, false otherwise.
     */
    inline bool inList(const std::vector<Vertex> &list) {
      for (auto const &v : list) {
        if (v.point == point)
          return true;
      }
      return false;
    }
  };

} // namespace quakelib