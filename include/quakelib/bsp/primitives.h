#pragma once
#include "bsp_file.h"
#include <memory>

namespace quakelib::bsp {
  struct Vertex {
    vec3f_t point;
    vec3f_t normal;
    vec2f_t uv;
    vec2f_t lm_uv;
  };

  class Surface {
  public:
    void Build(const bspFileContent &ctx, const fFace_t *fsurface);
    int id;
    int lightmapID;
    const fSurfaceInfo_t *info;
    const fFace_t *fsurface;
    const miptex_t *textureReference;
    vector<Vertex> verts;
    vector<uint32_t> indices;
    uint8_t *lm_samples;
    int lm_tex_num;
    int extents[2];
    int texturemins[2];
    short lm_s;
    short lm_t;

  private:
    void calculateSurfaceExtents();
  };

  typedef std::shared_ptr<Surface> SurfacePtr;
} // namespace quakelib::bsp