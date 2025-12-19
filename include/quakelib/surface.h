#pragma once

#include <quakelib/vertex.h>
#include <vector>

namespace quakelib {
  enum class SurfaceType {
    SOLID,
    CLIP,
    NODRAW,
  };

  class Surface {
  public:
    const std::vector<Vertex> &Vertices() const;
    const std::vector<uint32_t> &Indices() const;
    int TextureID() const;
    SurfaceType Type();

  protected:
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    int m_textureID;
    SurfaceType m_surfType;
  };
} // namespace quakelib