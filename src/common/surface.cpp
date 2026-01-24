#include <quakelib/surface.h>

namespace quakelib {
  const std::vector<Vertex> &Surface::Vertices() const { return m_vertices; }

  std::vector<Vertex> &Surface::VerticesRW() { return m_vertices; }

  const std::vector<uint32_t> &Surface::Indices() const { return m_indices; }

  int Surface::TextureID() const { return m_textureID; }

  SurfaceType Surface::Type() { return SurfaceType(); }
}
