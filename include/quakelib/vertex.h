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

  struct Vertex {
    fvec3 point;
    fvec3 normal;
    fvec2 uv;
    fvec2 lightmap_uv;
    fvec4 tangent;

    inline bool inList(const std::vector<Vertex> &list) {
      for (auto const &v : list) {
        if (v.point == point)
          return true;
      }
      return false;
    }
  };

} // namespace quakelib