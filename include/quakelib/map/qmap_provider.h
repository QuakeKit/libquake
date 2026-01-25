#pragma once
#include "map.h"
#include <quakelib/map_provider.h>

namespace quakelib {

  class QMapProvider : public IMapProvider {
  public:
    QMapProvider() = default;
    virtual ~QMapProvider() = default;

    bool Load(const std::string &path) override;
    void GenerateGeometry(bool csg = true) override;
    void SetFaceType(const std::string &textureName, SurfaceType type) override;

    std::vector<SolidEntityPtr> GetSolidEntities() const override;
    std::vector<PointEntityPtr> GetPointEntities() const override;
    std::vector<std::string> GetTextureNames() const override;

    std::vector<RenderMesh> GetEntityMeshes(const SolidEntityPtr &entity) override;

    std::vector<std::string> GetRequiredWads() const override;
    void SetTextureBoundsProvider(std::function<std::pair<int, int>(const std::string &)> provider) override;

  private:
    quakelib::map::QMap m_map;
    bool m_csg{true};
  };
} // namespace quakelib
