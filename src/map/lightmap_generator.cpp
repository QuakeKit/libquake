#include <algorithm>
#include <cmath>
#include <iostream>
#include <quakelib/map/lightmap_generator.h>
#include <quakelib/qmath.h>

namespace quakelib::map {

  LightmapGenerator::LightmapGenerator(int width, int height, float luxelSize)
      : m_width(width), m_height(height), m_luxelSize(luxelSize) {}

  bool LightmapGenerator::Pack(const std::vector<SolidEntityPtr> &entities) {
    m_entries.clear();

    for (const auto &ent : entities) {
      auto brushes = ent->GetClippedBrushes();
      if (brushes.empty())
        brushes = ent->Brushes();

      for (const auto &brush : brushes) {
        for (const auto &face : brush.Faces()) {
          if (face->Type() != MapSurface::SOLID)
            continue;

          Vec2 minUV = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
          Vec2 maxUV = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};

          for (const auto &v : face->Vertices()) {
            Vec2 uv = face->CalcLightmapUV(v.point);
            minUV[0] = std::min(minUV[0], uv[0]);
            minUV[1] = std::min(minUV[1], uv[1]);
            maxUV[0] = std::max(maxUV[0], uv[0]);
            maxUV[1] = std::max(maxUV[1], uv[1]);
          }

          int w = static_cast<int>(std::ceil((maxUV[0] - minUV[0]) / m_luxelSize)) + 1;
          int h = static_cast<int>(std::ceil((maxUV[1] - minUV[1]) / m_luxelSize)) + 1;

          w = std::max(1, w);
          h = std::max(1, h);

          LightmapEntry entry;
          entry.face = face;
          entry.w = w;
          entry.h = h;

          m_entries.push_back(entry);
        }
      }
    }

    std::sort(m_entries.begin(), m_entries.end(),
              [](const LightmapEntry &a, const LightmapEntry &b) { return a.h > b.h; });

    int currentX = 0;
    int currentY = 0;
    int rowHeight = 0;

    for (auto &entry : m_entries) {
      if (currentX + entry.w > m_width) {

        currentY += rowHeight;
        currentX = 0;
        rowHeight = 0;
      }

      if (currentY + entry.h > m_height) {
        std::cerr << "Lightmap Atlas full!" << std::endl;
        return false;
      }

      entry.x = currentX;
      entry.y = currentY;

      rowHeight = std::max(rowHeight, entry.h);

      currentX += entry.w;
    }

    for (const auto &entry : m_entries) {
      Vec2 minUV = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};

      for (const auto &v : entry.face->Vertices()) {
        Vec2 uv = entry.face->CalcLightmapUV(v.point);
        minUV[0] = std::min(minUV[0], uv[0]);
        minUV[1] = std::min(minUV[1], uv[1]);
      }

      auto &verts = entry.face->VerticesRW();
      for (auto &v : verts) {
        Vec2 localUV = entry.face->CalcLightmapUV(v.point);

        float u = (localUV[0] - minUV[0]) / m_luxelSize;
        float v_coord = (localUV[1] - minUV[1]) / m_luxelSize;

        u += entry.x;
        v_coord += entry.y;

        v.lightmap_uv[0] = u / (float)m_width;
        v.lightmap_uv[1] = v_coord / (float)m_height;
      }
    }

    GenerateAtlasImage();
    return true;
  }

  void LightmapGenerator::CalculateLighting(const std::vector<Light> &lights, Vec3 ambientColor) {

    unsigned char ambR = static_cast<unsigned char>(std::min(1.0f, ambientColor[0]) * 255);
    unsigned char ambG = static_cast<unsigned char>(std::min(1.0f, ambientColor[1]) * 255);
    unsigned char ambB = static_cast<unsigned char>(std::min(1.0f, ambientColor[2]) * 255);

    std::fill(m_data.begin(), m_data.end(), 255);
    for (int i = 0; i < m_width * m_height; ++i) {
      m_data[i * 4 + 0] = ambR;
      m_data[i * 4 + 1] = ambG;
      m_data[i * 4 + 2] = ambB;
      m_data[i * 4 + 3] = 255;
    }

    for (const auto &entry : m_entries) {
      Vec2 minUV = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
      for (const auto &v : entry.face->Vertices()) {
        Vec2 uv = entry.face->CalcLightmapUV(v.point);
        minUV[0] = std::min(minUV[0], uv[0]);
        minUV[1] = std::min(minUV[1], uv[1]);
      }

      Vec3 N = entry.face->GetPlaneNormal();

      for (int y = 0; y < entry.h; ++y) {
        for (int x = 0; x < entry.w; ++x) {

          float u_local = (minUV[0]) + (x * m_luxelSize) + (m_luxelSize * 0.5f);
          float v_local = (minUV[1]) + (y * m_luxelSize) + (m_luxelSize * 0.5f);

          Vec3 worldPos = entry.face->CalcWorldPosFromLightmapUV({u_local, v_local});

          worldPos += N * 0.5f;

          Vec3 totalLight = {0, 0, 0};

          for (const auto &light : lights) {
            Vec3 toLight = light.pos - worldPos;
            float dist = math::Len(toLight);

            if (dist > light.radius)
              continue;

            float attenuation = std::max(0.0f, 1.0f - (dist / light.radius));
            attenuation *= attenuation;

            Vec3 L = math::Norm(toLight);
            float nDotL = std::max(0.0f, math::Dot(N, L));

            totalLight += light.color * (nDotL * attenuation);
          }

          int atlasX = entry.x + x;
          int atlasY = entry.y + y;

          if (atlasX < m_width && atlasY < m_height) {
            int index = (atlasY * m_width + atlasX) * 4;

            int r = m_data[index + 0];
            int g = m_data[index + 1];
            int b = m_data[index + 2];

            r += static_cast<int>(totalLight[0] * 255.0f);
            g += static_cast<int>(totalLight[1] * 255.0f);
            b += static_cast<int>(totalLight[2] * 255.0f);

            m_data[index + 0] = std::min(255, r);
            m_data[index + 1] = std::min(255, g);
            m_data[index + 2] = std::min(255, b);
          }
        }
      }
    }
  }

  void LightmapGenerator::GenerateAtlasImage() {

    m_data.assign(m_width * m_height * 4, 127);

    for (const auto &entry : m_entries) {
      for (int y = entry.y; y < entry.y + entry.h; ++y) {
        for (int x = entry.x; x < entry.x + entry.w; ++x) {
          if (x >= m_width || y >= m_height)
            continue;

          int index = (y * m_width + x) * 4;

          bool border =
              (x == entry.x || x == entry.x + entry.w - 1 || y == entry.y || y == entry.y + entry.h - 1);

          if (border) {
            m_data[index + 0] = 0;
            m_data[index + 1] = 0;
            m_data[index + 2] = 0;
            m_data[index + 3] = 255;
          } else {

            bool check = ((x / 8) + (y / 8)) % 2 == 0;
            if (check) {
              m_data[index + 0] = 255;
              m_data[index + 1] = 255;
              m_data[index + 2] = 255;
            } else {
              m_data[index + 0] = 180;
              m_data[index + 1] = 180;
              m_data[index + 2] = 180;
            }
            m_data[index + 3] = 255;
          }
        }
      }
    }
  }

  const std::vector<unsigned char> &LightmapGenerator::GetAtlasData() const { return m_data; }

} // namespace quakelib::map
