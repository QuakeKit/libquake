#pragma once

#include "entity_solid.h"

namespace quakelib::bsp {
  const int LM_BLOCK_WIDTH = 256;
  const int LM_BLOCK_HEIGHT = 256;
  const int MAX_SANITY_LIGHTMAPS = (1u << 20);

  struct Color {
    union {
      struct {
        uint8_t r, g, b, a;
      };

      uint8_t rgba[4];
    };

    void Set(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
      this->r = r, this->g = g, this->b = b, this->a = a;
    };
  };

  struct LightmapChart {
    bool reverse;
    int x;
    int width;
    int height;
    int *allocated;
  };

  class Lightmap {
  public:
    Lightmap(uint8_t *data, size_t sz);
    void PackLitSurfaces(std::vector<SolidEntityPtr> ent);

    const int Width() const;
    const int Height() const;
    const vector<Color> &RGBA() const;

  private:
    void initChart(LightmapChart *chart, int width, int height);
    bool addChart(LightmapChart *chart, int w, int h, short *outx, short *outy);
    int allocateBlock(int w, int h, short *x, short *y);
    void fillSurfaceLightmap(SurfacePtr surf);

    LightmapChart m_chart;
    std::vector<vec2i_t> m_offsets;
    std::vector<SurfacePtr> m_litSurfs;

    uint8_t *m_rawData;
    vector<Color> m_lightmapData;
    size_t m_size = 0;
    int m_count = 0;
    int m_sampleCount = 0;
    int m_lastAllocated = 0;

    int m_width = 0;
    int m_height = 0;
  };
} // namespace quakelib::bsp