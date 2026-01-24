#pragma once

#include <quakelib/tue/vec.hpp>
#include <vector>

namespace quakelib {

  using tue::fvec2;
  using tue::fvec3;
  using tue::fvec4;

  /*
  using tue::math::cross;
  using tue::math::dot;
  using tue::math::normalize;
  */

  /**
   * @brief Represents a vertex in 3D space with associated attributes.
   */
  struct Vertex {
    fvec3 point;       ///< The 3D position of the vertex.
    fvec3 normal;      ///< The normal vector at this vertex.
    fvec2 uv;          ///< Texture coordinates.
    fvec2 lightmap_uv; ///< Lightmap coordinates.
    fvec4 tangent;     ///< Tangent vector (xyz) and bitangent sign (w).

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