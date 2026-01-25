#pragma once
#include "qbsp.h"
#include <memory>
#include <quakelib/map_provider.h>

namespace quakelib {

  class QBspProvider : public IMapProvider {
  public:
    QBspProvider();
    virtual ~QBspProvider() = default;

    bool Load(const std::string &path) override;
    bool Load(const std::string &path, const quakelib::bsp::QBspConfig &cfg);
    void GenerateGeometry(bool csg = true) override;
    void SetFaceType(const std::string &textureName, SurfaceType type) override;

    std::vector<SolidEntityPtr> GetSolidEntities() const override;
    std::vector<PointEntityPtr> GetPointEntities() const override;
    std::vector<std::string> GetTextureNames() const override;
    std::vector<RenderMesh> GetEntityMeshes(const SolidEntityPtr &entity) override;
    std::optional<TextureData> GetTextureData(const std::string &name) const override;
    std::optional<TextureData> GetLightmapData() const override;

  private:
    std::unique_ptr<quakelib::bsp::QBsp> m_bsp;
    std::map<std::string, SurfaceType> m_faceTypes;
  };
} // namespace quakelib
