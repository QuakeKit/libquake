#pragma once

#include <quakelib/tue/transform.hpp>

#include <array>
#include <iostream>
#include <map>
#include <quakelib/tue/vec.hpp>
#include <string>
#include <vector>

using tue::fvec2;
using tue::fvec3;
using tue::fvec4;

using tue::math::cross;
using tue::math::dot;
using tue::math::normalize;

inline float dist3(const fvec3 &a, const fvec3 &b) {
  fvec3 diff = b - a;
  return sqrtf(dot(diff, diff));
};

namespace quakelib::map {
  const double epsilon = 1e-5; // Used to compensate for floating point inaccuracy.
  const double scale = 128;    // Scale
                               // MAP FILE

  struct StandardUV {
    float u;
    float v;
  };

  struct ValveUV {
    fvec4 u;
    fvec4 v;
  };

  struct textureBounds {
    float width;
    float height;
  };
} // namespace quakelib::map
