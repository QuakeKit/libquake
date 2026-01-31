#pragma once

#include <array>
#include <iostream>
#include <map>
#include <quakelib/qmath.h>
#include <string>
#include <vector>

namespace quakelib::map {

  using math::Vec2;
  using math::Vec3;
  using math::Vec4;

  inline float dist3(const Vec3 &a, const Vec3 &b) {
    Vec3 diff = b - a;
    return sqrtf(HMM_Dot(diff, diff));
  };

  const double epsilon = 1e-5; // Used to compensate for floating point inaccuracy.
  const double scale = 128;    // Scale
                               // MAP FILE

  struct StandardUV {
    float u;
    float v;
  };

  struct ValveUV {
    Vec4 u;
    Vec4 v;
  };

  struct textureBounds {
    float width;
    float height;
  };
} // namespace quakelib::map
