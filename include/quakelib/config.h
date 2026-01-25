#pragma once

namespace quakelib {
  /**
   * @brief Base configuration structure for all format loaders.
   *
   * Provides common options that apply across different Quake file formats.
   */
  struct Config {
    /**
     * @brief Convert coordinates from Quake to OpenGL coordinate system.
     *
     * When enabled, converts from Quake's coordinate system (X=forward, Y=left, Z=up)
     * to OpenGL's coordinate system (X=right, Y=up, Z=backward).
     *
     * @note This affects vertices, normals, and entity positions.
     */
    bool convertCoordToOGL = false;
  };
} // namespace quakelib
