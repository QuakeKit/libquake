#pragma once

#include <quakelib/map/entities.h>
#include <vector>

namespace quakelib::map {

  struct LightmapEntry {
    int x, y; // Position in atlas
    int w, h; // Size in atlas
    FacePtr face;
  };

  class LightmapGenerator {
  public:
    LightmapGenerator(int width = 512, int height = 512, float luxelSize = 16.0f);

    // Packs all faces from the provided entities into the atlas
    // Returns false if the atlas is too small
    bool Pack(const std::vector<SolidEntityPtr> &entities);

    // Get the generated atlas data (RGBA or single channel)
    // Currently returns a simple debug pattern/white texture
    const std::vector<unsigned char> &GetAtlasData() const;

    int GetWidth() const { return m_width; }

    int GetHeight() const { return m_height; }

    struct Light {
      fvec3 pos;
      float radius;
      fvec3 color;
    };

    void CalculateLighting(const std::vector<Light> &lights,
                           fvec3 ambientColor = {30.0f / 255.0f, 30.0f / 255.0f, 30.0f / 255.0f});

  private:
    void GenerateAtlasImage();

    int m_width;
    int m_height;
    float m_luxelSize;
    std::vector<unsigned char> m_data;
    std::vector<LightmapEntry> m_entries;
  };

} // namespace quakelib::map
