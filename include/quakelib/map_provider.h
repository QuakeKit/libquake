#pragma once

#include "entities.h"
#include "vertex.h"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace quakelib {

  struct TextureData {
    int width;
    int height;
    std::vector<unsigned char> data; // RGBA
  };

  struct RenderMesh {
    std::string textureName;
    int textureWidth{0};
    int textureHeight{0};
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    SurfaceType type{SurfaceType::SOLID};
  };

  class IMapProvider {
  public:
    virtual ~IMapProvider() = default;

    virtual bool Load(const std::string &path) = 0;
    virtual void GenerateGeometry(bool csg = true) = 0;
    virtual void SetFaceType(const std::string &textureName, SurfaceType type) = 0;

    virtual std::vector<SolidEntityPtr> GetSolidEntities() const = 0;
    virtual std::vector<PointEntityPtr> GetPointEntities() const = 0;
    virtual std::vector<std::string> GetTextureNames() const = 0;

    virtual std::vector<RenderMesh> GetEntityMeshes(const SolidEntityPtr &entity) = 0;

    virtual std::vector<std::string> GetRequiredWads() const { return {}; }

    virtual void SetTextureBoundsProvider(std::function<std::pair<int, int>(const std::string &)> provider) {}

    virtual std::optional<TextureData> GetTextureData(const std::string &name) const { return std::nullopt; }

    virtual std::optional<TextureData> GetLightmapData() const { return std::nullopt; }
  };

  using IMapProviderPtr = std::shared_ptr<IMapProvider>;

} // namespace quakelib
