#pragma once
#include "map.h"
#include <quakelib/map_provider.h>

namespace quakelib {

  /**
   * @brief Map provider implementation for MAP source files.
   *
   * QMapProvider implements the IMapProvider interface to load Quake
   * source map files (.map). It handles parsing, CSG operations, and
   * geometry generation for editable map files.
   *
   * This allows applications to work with both source MAP files and
   * compiled BSP files using the same interface.
   *
   * @see IMapProvider
   * @see quakelib::map::QMap
   */
  class QMapProvider : public IMapProvider {
  public:
    /**
     * @brief Default constructor.
     */
    QMapProvider() = default;

    /**
     * @brief Virtual destructor.
     */
    virtual ~QMapProvider() = default;

    /**
     * @brief Load a MAP file with default configuration.
     * @param path Path to the .map file.
     * @return True on success, false on failure.
     */
    bool Load(const std::string &path) override;

    /**
     * @brief Load a MAP file with custom configuration.
     * @param path Path to the .map file.
     * @param cfg Configuration options for loading and processing.
     * @return True on success, false on failure.
     */
    bool Load(const std::string &path, const quakelib::map::QMapConfig &cfg);
    void GenerateGeometry(bool csg = true) override;
    void SetFaceType(const std::string &textureName, SurfaceType type) override;

    std::vector<SolidEntityPtr> GetSolidEntities() const override;
    std::vector<SolidEntityPtr> GetSolidEntities(const std::string &className) const override;
    std::vector<PointEntityPtr> GetPointEntities() const override;
    std::vector<PointEntityPtr> GetPointEntities(const std::string &className) const override;
    std::vector<std::string> GetTextureNames() const override;

    std::vector<RenderMesh> GetEntityMeshes(const SolidEntityPtr &entity) override;

    std::vector<std::string> GetRequiredWads() const override;
    void SetTextureBoundsProvider(std::function<std::pair<int, int>(const std::string &)> provider) override;

  private:
    void weldVertices(RenderMesh &mesh, const std::vector<quakelib::map::FacePtr> &faces);
    void generateLightmapUVs(std::vector<RenderMesh> &meshes);

    quakelib::map::QMap m_map;
  };
} // namespace quakelib
