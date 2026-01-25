//
// Created by tgoeh on 10.11.2023.
//
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "brush.h"
#include "entities.h"
#include "map_file.h"
#include "types.h"

namespace quakelib::map {
  /**
   * @brief Callback type for gathering polygons.
   * @param faces Vector of faces collected.
   * @param textureID The texture ID associated with these faces (or -1/generic).
   */
  using polygonGatherCb = std::function<void(std::vector<FacePtr>, int)>;

  /**
   * @brief Callback type for retrieving texture dimensions.
   * @param textureName The name of the texture.
   * @return The bounding dimensions of the texture.
   */
  using getTextureBoundsCb = std::function<textureBounds(const char *textureName)>;

  /**
   * @brief High-level class for loading and processing Quake maps.
   *
   * QMap handles file loading, geometry generation, and provides access to entities and map data.
   */
  class QMap {
  public:
    /**
     * @brief Default constructor.
     */
    QMap() = default;

    /**
     * @brief Default destructor.
     */
    ~QMap() = default;

    /**
     * @brief Loads a map from a file on disk.
     * @param filename Path to the map file.
     * @param getTextureBounds Optional callback to retrieve texture sizes for UV calculations.
     */
    void LoadFile(const std::string &filename, getTextureBoundsCb getTextureBounds = nullptr);

    /**
     * @brief Loads a map from a string buffer.
     * @param buffer Null-terminated string containing the map data.
     * @param getTextureBounds Callback to retrieve texture sizes.
     */
    void LoadBuffer(const char *buffer, getTextureBoundsCb getTextureBounds);

    /**
     * @brief Registers a callback for texture bounds retrieval.
     *
     * This is useful if texture data becomes available after loading but before geometry generation.
     * @param getTextureBounds The callback function.
     */
    void RegisterTextureBounds(getTextureBoundsCb getTextureBounds);

    /**
     * @brief Generates standard geometry (CSG, triangulation) for the map brushes.
     * @param clipBrushes If true, performs CSG clipping (intersections).
     */
    void GenerateGeometry(bool clipBrushes = true);

    /**
     * @brief Collects polygons for a specific entity.
     * @param entityID The ID of the entity to process.
     * @param cb Callback invoked with the resulting faces.
     */
    void GatherPolygons(int entityID, const polygonGatherCb &cb);

    /**
     * @brief Retrieves faces for an entity filtered by texture.
     * @param entityID The ID of the entity.
     * @param texName The name of the texture to filter by.
     * @return A vector of pointers to matching faces.
     */
    std::vector<FacePtr> PolygonsByTexture(int entityID, const std::string &texName);

    /**
     * @brief Gets the list of WAD files referenced by the map.
     * @return Vector of WAD filenames.
     */
    const std::vector<std::string> &Wads() const { return m_map_file->m_wads; };

    /**
     * @brief Checks if the map references any WAD files.
     * @return True if WADs are present, false otherwise.
     */
    bool HasWads() const { return !m_map_file->m_wads.empty(); };

    /**
     * @brief Gets all texture names used in the map.
     * @return Vector of texture names.
     */
    const std::vector<std::string> &TextureNames() const { return m_map_file->m_textures; };

    /**
     * @brief Gets a texture name by its internal ID.
     * @param textureID The numeric ID of the texture.
     * @return The name of the texture.
     */
    const std::string &TextureName(int textureID);

    /**
     * @brief Accesses the low-level map file data structure.
     * @return Pointer to the internal QMapFile.
     */
    QMapFile *MapData() { return m_map_file.get(); };

    /**
     * @brief Overrides the face type for a specific texture.
     *
     * Useful for assigning special properties (e.g., sky, liquids) based on texture name.
     * @param texture The texture name.
     * @param type The new face type to assign.
     */
    void SetFaceTypeByTextureID(const std::string &texture, MapSurface::eFaceType type);

    /**
     * @brief Gets the WorldSpawn entity (the static world).
     * @return Pointer to the WorldSpawn entity.
     */
    const SolidMapEntity *WorldSpawn() { return m_map_file->m_worldSpawn; }

    /**
     * @brief Gets the list of solid (brush-based) entities.
     * @return Vector of pointers to solid entities.
     */
    const std::vector<SolidEntityPtr> &SolidEntities() const { return m_map_file->m_solidEntities; };

    /**
     * @brief Gets the list of point entities.
     * @return Vector of pointers to point entities.
     */
    const std::vector<PointEntityPtr> &PointEntities() const { return m_map_file->m_pointEntities; };

    /**
     * @brief Finds point entities by their classname.
     * @param className The classname to search for (case sensitive usually).
     * @return Vector of matching point entites.
     */
    std::vector<PointEntityPtr> PointEntitiesByClass(const std::string &className);

  private:
    bool getPolygonsByTextureID(int entityID, int texID, std::vector<FacePtr> &list);

    std::map<int, MapSurface::eFaceType> m_textureIDTypes;
    std::map<int, textureBounds> m_textureIDBounds;
    std::shared_ptr<QMapFile> m_map_file;
  };
} // namespace quakelib::map
