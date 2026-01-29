#include "wrapper.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <memory>
#include <quakelib/bsp/qbsp_provider.h>
#include <quakelib/map/lightmap_generator.h>
#include <quakelib/map/qmap_provider.h>
#include <quakelib/wad/wad.h>

// ============================================================================
// Helper Functions
// ============================================================================

// Global map for storing texture sizes per provider instance
static std::map<void *, std::map<std::string, std::pair<int, int>>> g_providerTextureSizes;

static QLibVec2 ToQLibVec2(const quakelib::fvec2 &v) { return {v[0], v[1]}; }

static QLibVec3 ToQLibVec3(const quakelib::fvec3 &v) { return {v[0], v[1], v[2]}; }

static QLibVec4 ToQLibVec4(const quakelib::fvec4 &v) { return {v[0], v[1], v[2], v[3]}; }

static QLibVertex ToQLibVertex(const quakelib::Vertex &v) {
  QLibVertex out;
  out.position = ToQLibVec3(v.point);
  out.normal = ToQLibVec3(v.normal);
  out.tangent = ToQLibVec4(v.tangent);
  out.uv = ToQLibVec2(v.uv);
  out.lightmapUV = ToQLibVec2(v.lightmap_uv);
  return out;
}

static void SafeStrCopy(char *dest, const std::string &src, size_t maxLen) {
  size_t len = std::min(src.length(), maxLen - 1);
  std::memcpy(dest, src.c_str(), len);
  dest[len] = '\0';
}

static char **AllocStringArray(const std::vector<std::string> &strings) {
  if (strings.empty())
    return nullptr;

  char **arr = (char **)QLib_Malloc(strings.size() * sizeof(char *));
  for (size_t i = 0; i < strings.size(); i++) {
    arr[i] = (char *)QLib_Malloc(strings[i].length() + 1);
    std::strcpy(arr[i], strings[i].c_str());
  }
  return arr;
}

static void FreeStringArray(char **arr, uint32_t count) {
  if (!arr)
    return;
  for (uint32_t i = 0; i < count; i++) {
    QLib_Free(arr[i]);
  }
  QLib_Free(arr);
}

static void ExportAttributes(const quakelib::Entity *entity, uint32_t *outCount, char ***outKeys,
                             char ***outValues) {
  const auto &attrs = entity->Attributes();
  // Add 1 for classname which is stored separately
  size_t totalAttrs = attrs.size() + 1;
  *outCount = static_cast<uint32_t>(totalAttrs);

  *outKeys = (char **)QLib_Malloc(totalAttrs * sizeof(char *));
  *outValues = (char **)QLib_Malloc(totalAttrs * sizeof(char *));

  // First add classname
  const std::string &className = entity->ClassName();
  (*outKeys)[0] = (char *)QLib_Malloc(10); // "classname"
  (*outValues)[0] = (char *)QLib_Malloc(className.length() + 1);
  std::strcpy((*outKeys)[0], "classname");
  std::strcpy((*outValues)[0], className.c_str());

  // Then add other attributes
  size_t idx = 1;
  for (const auto &[key, value] : attrs) {
    (*outKeys)[idx] = (char *)QLib_Malloc(key.length() + 1);
    (*outValues)[idx] = (char *)QLib_Malloc(value.length() + 1);
    std::strcpy((*outKeys)[idx], key.c_str());
    std::strcpy((*outValues)[idx], value.c_str());
    idx++;
  }
}

// ============================================================================
// WAD Implementation
// ============================================================================

API_EXPORT void *QLibWad_Load(const char *filePath, uint8_t flipHorizontal) {
  quakelib::wad::QuakeWadOptions opts;
  opts.flipTexHorizontal = flipHorizontal != 0;

  auto wad = quakelib::wad::QuakeWad::FromFile(filePath, opts);
  if (!wad)
    return nullptr;

  return new quakelib::wad::QuakeWadPtr(wad);
}

API_EXPORT QLibWadData *QLibWad_ExportAll(void *wadPtr) {
  if (!wadPtr)
    return nullptr;

  auto &wad = *static_cast<quakelib::wad::QuakeWadPtr *>(wadPtr);
  const auto &textures = wad->Textures();

  auto *data = (QLibWadData *)QLib_Malloc(sizeof(QLibWadData));
  data->textureCount = static_cast<uint32_t>(textures.size());
  data->textures = (QLibWadTexture *)QLib_Malloc(data->textureCount * sizeof(QLibWadTexture));

  size_t idx = 0;
  for (const auto &[name, entry] : textures) {
    auto &outTex = data->textures[idx++];
    SafeStrCopy(outTex.name, name, 16);
    outTex.width = entry.texture.width;
    outTex.height = entry.texture.height;
    outTex.isSky = (entry.Type() == quakelib::wad::QuakeWadEntry::QWET_SBarPic)
                       ? 0
                       : (quakelib::wad::QuakeWad::IsSkyTexture(name) ? 1 : 0);

    // Convert color vector to RGBA bytes
    size_t numPixels = entry.texture.raw.size();
    outTex.dataSize = static_cast<uint32_t>(numPixels * 4); // RGBA

    if (numPixels > 0) {
      outTex.data = (uint8_t *)QLib_Malloc(outTex.dataSize);
      for (size_t i = 0; i < numPixels; i++) {
        std::memcpy(&outTex.data[i * 4], entry.texture.raw[i].rgba, 4);
      }
    } else {
      outTex.data = nullptr;
    }
  }

  return data;
}

API_EXPORT QLibWadTexture *QLibWad_GetTexture(void *wadPtr, const char *name) {
  if (!wadPtr || !name)
    return nullptr;

  auto &wad = *static_cast<quakelib::wad::QuakeWadPtr *>(wadPtr);
  auto *texture = wad->GetTexture(name);
  if (!texture)
    return nullptr;

  auto *outTex = (QLibWadTexture *)QLib_Malloc(sizeof(QLibWadTexture));
  SafeStrCopy(outTex->name, name, 16);
  outTex->width = texture->width;
  outTex->height = texture->height;
  outTex->isSky = quakelib::wad::QuakeWad::IsSkyTexture(name) ? 1 : 0;

  // Convert color vector to RGBA bytes
  size_t numPixels = texture->raw.size();
  outTex->dataSize = static_cast<uint32_t>(numPixels * 4); // RGBA

  if (numPixels > 0) {
    outTex->data = (uint8_t *)QLib_Malloc(outTex->dataSize);
    for (size_t i = 0; i < numPixels; i++) {
      std::memcpy(&outTex->data[i * 4], texture->raw[i].rgba, 4);
    }
  } else {
    outTex->data = nullptr;
  }

  return outTex;
}

API_EXPORT void QLibWad_FreeTexture(QLibWadTexture *texture) {
  if (!texture)
    return;
  if (texture->data)
    QLib_Free(texture->data);
  QLib_Free(texture);
}

API_EXPORT void QLibWad_FreeData(QLibWadData *data) {
  if (!data)
    return;

  for (uint32_t i = 0; i < data->textureCount; i++) {
    if (data->textures[i].data) {
      QLib_Free(data->textures[i].data);
    }
  }
  QLib_Free(data->textures);
  QLib_Free(data);
}

API_EXPORT void QLibWad_Destroy(void *wadPtr) {
  if (!wadPtr)
    return;
  delete static_cast<quakelib::wad::QuakeWadPtr *>(wadPtr);
}

// ============================================================================
// BSP Implementation
// ============================================================================

API_EXPORT void *QLibBsp_Load(const char *filePath, uint8_t loadTextures, uint8_t loadTextureData,
                              uint8_t convertToOpenGL) {
  auto *provider = new quakelib::QBspProvider();

  quakelib::bsp::QBspConfig cfg;
  cfg.loadTextures = loadTextures != 0;
  cfg.loadTextureData = loadTextureData != 0;
  cfg.convertCoordToOGL = convertToOpenGL != 0;

  if (!provider->Load(filePath, cfg)) {
    delete provider;
    return nullptr;
  }

  return provider;
}

API_EXPORT QLibBspData *QLibBsp_ExportAll(void *bspPtr) {
  if (!bspPtr)
    return nullptr;

  auto *provider = static_cast<quakelib::QBspProvider *>(bspPtr);
  auto solidEntities = provider->GetSolidEntities();
  auto pointEntities = provider->GetPointEntities();
  auto textureNames = provider->GetTextureNames();

  auto *data = (QLibBspData *)QLib_Malloc(sizeof(QLibBspData));
  std::memset(data, 0, sizeof(QLibBspData));

  // Export textures
  data->textureCount = static_cast<uint32_t>(textureNames.size());
  if (data->textureCount > 0) {
    data->textures = (QLibBspTexture *)QLib_Malloc(data->textureCount * sizeof(QLibBspTexture));
    std::memset(data->textures, 0, data->textureCount * sizeof(QLibBspTexture));

    for (uint32_t i = 0; i < data->textureCount; i++) {
      auto &outTex = data->textures[i];
      SafeStrCopy(outTex.name, textureNames[i], 64);

      auto texData = provider->GetTextureData(textureNames[i]);
      if (texData) {
        outTex.width = texData->width;
        outTex.height = texData->height;
        outTex.dataSize = static_cast<uint32_t>(texData->data.size());

        if (outTex.dataSize > 0) {
          outTex.data = (uint8_t *)QLib_Malloc(outTex.dataSize);
          std::memcpy(outTex.data, texData->data.data(), outTex.dataSize);
        }
      }
    }
  }

  // Export solid entities
  data->solidEntityCount = static_cast<uint32_t>(solidEntities.size());
  if (data->solidEntityCount > 0) {
    data->solidEntities =
        (QLibBspEntityMesh *)QLib_Malloc(data->solidEntityCount * sizeof(QLibBspEntityMesh));

    for (uint32_t i = 0; i < data->solidEntityCount; i++) {
      auto &entity = solidEntities[i];
      auto &outMesh = data->solidEntities[i];
      std::memset(&outMesh, 0, sizeof(QLibBspEntityMesh));

      SafeStrCopy(outMesh.className, entity->ClassName(), 64);

      // Get meshes
      auto meshes = provider->GetEntityMeshes(entity);
      outMesh.submeshCount = static_cast<uint32_t>(meshes.size());

      // Count total vertices/indices
      uint32_t totalVerts = 0;
      uint32_t totalIndices = 0;
      for (const auto &mesh : meshes) {
        totalVerts += static_cast<uint32_t>(mesh.vertices.size());
        totalIndices += static_cast<uint32_t>(mesh.indices.size());
      }

      outMesh.totalVertexCount = totalVerts;
      outMesh.totalIndexCount = totalIndices;

      // Allocate buffers
      if (totalVerts > 0) {
        outMesh.vertices = (QLibVertex *)QLib_Malloc(totalVerts * sizeof(QLibVertex));
      }
      if (totalIndices > 0) {
        outMesh.indices = (uint32_t *)QLib_Malloc(totalIndices * sizeof(uint32_t));
      }
      if (outMesh.submeshCount > 0) {
        outMesh.submeshes = (QLibBspSubmesh *)QLib_Malloc(outMesh.submeshCount * sizeof(QLibBspSubmesh));
      }

      // Fill data
      uint32_t vertOffset = 0;
      uint32_t idxOffset = 0;

      for (uint32_t j = 0; j < outMesh.submeshCount; j++) {
        const auto &mesh = meshes[j];
        auto &submesh = outMesh.submeshes[j];

        submesh.vertexOffset = vertOffset;
        submesh.vertexCount = static_cast<uint32_t>(mesh.vertices.size());
        submesh.indexOffset = idxOffset;
        submesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
        submesh.textureIndex = -1;
        SafeStrCopy(submesh.textureName, mesh.textureName, 64);

        // Find texture index
        for (uint32_t k = 0; k < data->textureCount; k++) {
          if (mesh.textureName == data->textures[k].name) {
            submesh.textureIndex = k;
            break;
          }
        }

        // Copy vertices
        for (uint32_t k = 0; k < submesh.vertexCount; k++) {
          outMesh.vertices[vertOffset + k] = ToQLibVertex(mesh.vertices[k]);
        }

        // Copy indices
        for (uint32_t k = 0; k < submesh.indexCount; k++) {
          outMesh.indices[idxOffset + k] = mesh.indices[k] + vertOffset;
        }

        vertOffset += submesh.vertexCount;
        idxOffset += submesh.indexCount;
      }
    }
  }

  // Export point entities
  data->pointEntityCount = static_cast<uint32_t>(pointEntities.size());
  if (data->pointEntityCount > 0) {
    data->pointEntities =
        (QLibBspPointEntity *)QLib_Malloc(data->pointEntityCount * sizeof(QLibBspPointEntity));

    for (uint32_t i = 0; i < data->pointEntityCount; i++) {
      auto &entity = pointEntities[i];
      auto &outEnt = data->pointEntities[i];
      std::memset(&outEnt, 0, sizeof(QLibBspPointEntity));

      SafeStrCopy(outEnt.className, entity->ClassName(), 64);
      outEnt.origin = ToQLibVec3(entity->Origin());
      outEnt.angle = entity->Angle();

      ExportAttributes(entity.get(), &outEnt.attributeCount, &outEnt.attributeKeys, &outEnt.attributeValues);
    }
  }

  return data;
}

API_EXPORT QLibBspEntityMesh *QLibBsp_GetEntityMesh(void *bspPtr, uint32_t entityIndex) {
  if (!bspPtr)
    return nullptr;

  auto *provider = static_cast<quakelib::QBspProvider *>(bspPtr);
  auto solidEntities = provider->GetSolidEntities();

  if (entityIndex >= solidEntities.size())
    return nullptr;

  auto &entity = solidEntities[entityIndex];
  auto meshes = provider->GetEntityMeshes(entity);

  auto *outMesh = (QLibBspEntityMesh *)QLib_Malloc(sizeof(QLibBspEntityMesh));
  std::memset(outMesh, 0, sizeof(QLibBspEntityMesh));

  SafeStrCopy(outMesh->className, entity->ClassName(), 64);

  // Count totals
  uint32_t totalVerts = 0;
  uint32_t totalIndices = 0;
  for (const auto &mesh : meshes) {
    totalVerts += static_cast<uint32_t>(mesh.vertices.size());
    totalIndices += static_cast<uint32_t>(mesh.indices.size());
  }

  outMesh->totalVertexCount = totalVerts;
  outMesh->totalIndexCount = totalIndices;
  outMesh->submeshCount = static_cast<uint32_t>(meshes.size());

  if (totalVerts > 0)
    outMesh->vertices = (QLibVertex *)QLib_Malloc(totalVerts * sizeof(QLibVertex));
  if (totalIndices > 0)
    outMesh->indices = (uint32_t *)QLib_Malloc(totalIndices * sizeof(uint32_t));
  if (outMesh->submeshCount > 0)
    outMesh->submeshes = (QLibBspSubmesh *)QLib_Malloc(outMesh->submeshCount * sizeof(QLibBspSubmesh));

  uint32_t vertOffset = 0;
  uint32_t idxOffset = 0;

  for (uint32_t i = 0; i < outMesh->submeshCount; i++) {
    const auto &mesh = meshes[i];
    auto &submesh = outMesh->submeshes[i];

    submesh.vertexOffset = vertOffset;
    submesh.vertexCount = static_cast<uint32_t>(mesh.vertices.size());
    submesh.indexOffset = idxOffset;
    submesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
    submesh.textureIndex = -1;
    SafeStrCopy(submesh.textureName, mesh.textureName, 64);

    for (uint32_t j = 0; j < submesh.vertexCount; j++) {
      outMesh->vertices[vertOffset + j] = ToQLibVertex(mesh.vertices[j]);
    }

    for (uint32_t j = 0; j < submesh.indexCount; j++) {
      outMesh->indices[idxOffset + j] = mesh.indices[j] + vertOffset;
    }

    vertOffset += submesh.vertexCount;
    idxOffset += submesh.indexCount;
  }

  return outMesh;
}

API_EXPORT void QLibBsp_FreeMesh(QLibBspEntityMesh *mesh) {
  if (!mesh)
    return;

  if (mesh->vertices)
    QLib_Free(mesh->vertices);
  if (mesh->indices)
    QLib_Free(mesh->indices);
  if (mesh->submeshes)
    QLib_Free(mesh->submeshes);
  QLib_Free(mesh);
}

API_EXPORT void QLibBsp_FreeData(QLibBspData *data) {
  if (!data)
    return;

  // Free textures
  if (data->textures) {
    for (uint32_t i = 0; i < data->textureCount; i++) {
      if (data->textures[i].data)
        QLib_Free(data->textures[i].data);
    }
    QLib_Free(data->textures);
  }

  // Free solid entities
  if (data->solidEntities) {
    for (uint32_t i = 0; i < data->solidEntityCount; i++) {
      auto &mesh = data->solidEntities[i];
      if (mesh.vertices)
        QLib_Free(mesh.vertices);
      if (mesh.indices)
        QLib_Free(mesh.indices);
      if (mesh.submeshes)
        QLib_Free(mesh.submeshes);
    }
    QLib_Free(data->solidEntities);
  }

  // Free point entities
  if (data->pointEntities) {
    for (uint32_t i = 0; i < data->pointEntityCount; i++) {
      auto &ent = data->pointEntities[i];
      FreeStringArray(ent.attributeKeys, ent.attributeCount);
      FreeStringArray(ent.attributeValues, ent.attributeCount);
    }
    QLib_Free(data->pointEntities);
  }

  // Free lightmap
  if (data->lightmapData)
    QLib_Free(data->lightmapData);

  QLib_Free(data);
}

API_EXPORT void QLibBsp_Destroy(void *bspPtr) {
  if (!bspPtr)
    return;
  delete static_cast<quakelib::QBspProvider *>(bspPtr);
}

// ============================================================================
// MAP Implementation
// ============================================================================

API_EXPORT void *QLibMap_Load(const char *filePath, uint8_t enableCSG, uint8_t convertToOpenGL) {
  auto *provider = new quakelib::QMapProvider();

  quakelib::map::QMapConfig cfg;
  cfg.csg = enableCSG != 0;
  cfg.convertCoordToOGL = convertToOpenGL != 0;

  if (!provider->Load(filePath, cfg)) {
    delete provider;
    return nullptr;
  }

  return provider;
}

API_EXPORT char **QLibMap_GetRequiredWads(void *mapPtr, uint32_t *outCount) {
  if (!mapPtr || !outCount)
    return nullptr;

  auto *provider = static_cast<quakelib::QMapProvider *>(mapPtr);
  auto wads = provider->GetRequiredWads();

  *outCount = static_cast<uint32_t>(wads.size());
  if (wads.empty())
    return nullptr;

  return AllocStringArray(wads);
}

API_EXPORT char **QLibMap_GetTextureNames(void *mapPtr, uint32_t *outCount) {
  if (!mapPtr || !outCount)
    return nullptr;

  auto *provider = static_cast<quakelib::QMapProvider *>(mapPtr);
  auto textures = provider->GetTextureNames();

  *outCount = static_cast<uint32_t>(textures.size());
  if (textures.empty())
    return nullptr;

  return AllocStringArray(textures);
}

API_EXPORT void QLibMap_RegisterTextureSize(void *mapPtr, const char *textureName, uint32_t width,
                                            uint32_t height) {
  if (!mapPtr || !textureName)
    return;

  auto &providerTextures = g_providerTextureSizes[mapPtr];
  providerTextures[textureName] = {static_cast<int>(width), static_cast<int>(height)};
}

API_EXPORT void QLibMap_GenerateGeometry(void *mapPtr) {
  if (!mapPtr)
    return;

  auto *provider = static_cast<quakelib::QMapProvider *>(mapPtr);

  auto it = g_providerTextureSizes.find(mapPtr);
  if (it != g_providerTextureSizes.end() && !it->second.empty()) {
    provider->SetTextureBoundsProvider([mapPtr](const std::string &name) -> std::pair<int, int> {
      auto &textureSizes = g_providerTextureSizes[mapPtr];
      auto it = textureSizes.find(name);
      if (it != textureSizes.end()) {
        return it->second;
      }
      return {0, 0};
    });
  }

  provider->GenerateGeometry();
}

API_EXPORT QLibMapData *QLibMap_ExportAll(void *mapPtr) {
  if (!mapPtr)
    return nullptr;

  auto *provider = static_cast<quakelib::QMapProvider *>(mapPtr);
  auto solidEntities = provider->GetSolidEntities();
  auto pointEntities = provider->GetPointEntities();
  auto textureNames = provider->GetTextureNames();
  auto requiredWads = provider->GetRequiredWads();

  auto *data = (QLibMapData *)QLib_Malloc(sizeof(QLibMapData));
  std::memset(data, 0, sizeof(QLibMapData));

  // Export textures
  data->textureCount = static_cast<uint32_t>(textureNames.size());
  data->textureNames = AllocStringArray(textureNames);

  // Export required WADs
  data->requiredWadCount = static_cast<uint32_t>(requiredWads.size());
  data->requiredWads = AllocStringArray(requiredWads);

  // Export solid entities
  data->solidEntityCount = static_cast<uint32_t>(solidEntities.size());
  if (data->solidEntityCount > 0) {
    data->solidEntities =
        (QLibMapEntityMesh *)QLib_Malloc(data->solidEntityCount * sizeof(QLibMapEntityMesh));

    for (uint32_t i = 0; i < data->solidEntityCount; i++) {
      auto &entity = solidEntities[i];
      auto &outMesh = data->solidEntities[i];
      std::memset(&outMesh, 0, sizeof(QLibMapEntityMesh));

      SafeStrCopy(outMesh.className, entity->ClassName(), 64);

      // Set bounds (cast to SolidMapEntity to access bounds methods)
      auto mapEntity = std::static_pointer_cast<quakelib::map::SolidMapEntity>(entity);
      auto center = mapEntity->GetCenter();
      auto boundsMin = mapEntity->GetMin();
      auto boundsMax = mapEntity->GetMax();
      outMesh.center = {center.x(), center.y(), center.z()};
      outMesh.boundsMin = {boundsMin.x(), boundsMin.y(), boundsMin.z()};
      outMesh.boundsMax = {boundsMax.x(), boundsMax.y(), boundsMax.z()};

      // Get meshes
      auto meshes = provider->GetEntityMeshes(entity);
      outMesh.submeshCount = static_cast<uint32_t>(meshes.size());

      // Count totals
      uint32_t totalVerts = 0;
      uint32_t totalIndices = 0;
      for (const auto &mesh : meshes) {
        totalVerts += static_cast<uint32_t>(mesh.vertices.size());
        totalIndices += static_cast<uint32_t>(mesh.indices.size());
      }

      outMesh.totalVertexCount = totalVerts;
      outMesh.totalIndexCount = totalIndices;

      // Allocate
      if (totalVerts > 0)
        outMesh.vertices = (QLibVertex *)QLib_Malloc(totalVerts * sizeof(QLibVertex));
      if (totalIndices > 0)
        outMesh.indices = (uint32_t *)QLib_Malloc(totalIndices * sizeof(uint32_t));
      if (outMesh.submeshCount > 0)
        outMesh.submeshes = (QLibMapSubmesh *)QLib_Malloc(outMesh.submeshCount * sizeof(QLibMapSubmesh));

      // Fill
      uint32_t vertOffset = 0;
      uint32_t idxOffset = 0;

      for (uint32_t j = 0; j < outMesh.submeshCount; j++) {
        const auto &mesh = meshes[j];
        auto &submesh = outMesh.submeshes[j];

        submesh.vertexOffset = vertOffset;
        submesh.vertexCount = static_cast<uint32_t>(mesh.vertices.size());
        submesh.indexOffset = idxOffset;
        submesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
        submesh.textureID = -1;
        SafeStrCopy(submesh.textureName, mesh.textureName, 64);
        submesh.surfaceType = static_cast<uint8_t>(mesh.type);

        // Find texture ID
        for (uint32_t k = 0; k < data->textureCount; k++) {
          if (mesh.textureName == textureNames[k]) {
            submesh.textureID = k;
            break;
          }
        }

        // Copy vertices
        for (uint32_t k = 0; k < submesh.vertexCount; k++) {
          outMesh.vertices[vertOffset + k] = ToQLibVertex(mesh.vertices[k]);
        }

        // Copy indices
        for (uint32_t k = 0; k < submesh.indexCount; k++) {
          outMesh.indices[idxOffset + k] = mesh.indices[k] + vertOffset;
        }

        vertOffset += submesh.vertexCount;
        idxOffset += submesh.indexCount;
      }

      // Export attributes
      ExportAttributes(entity.get(), &outMesh.attributeCount, &outMesh.attributeKeys,
                       &outMesh.attributeValues);
    }
  }

  // Export point entities
  data->pointEntityCount = static_cast<uint32_t>(pointEntities.size());
  if (data->pointEntityCount > 0) {
    data->pointEntities =
        (QLibMapPointEntity *)QLib_Malloc(data->pointEntityCount * sizeof(QLibMapPointEntity));

    for (uint32_t i = 0; i < data->pointEntityCount; i++) {
      auto &entity = pointEntities[i];
      auto &outEnt = data->pointEntities[i];
      std::memset(&outEnt, 0, sizeof(QLibMapPointEntity));

      SafeStrCopy(outEnt.className, entity->ClassName(), 64);
      outEnt.origin = ToQLibVec3(entity->Origin());
      outEnt.angle = entity->Angle();

      ExportAttributes(entity.get(), &outEnt.attributeCount, &outEnt.attributeKeys, &outEnt.attributeValues);
    }
  }

  return data;
}

API_EXPORT QLibMapEntityMesh *QLibMap_GetEntityMesh(void *mapPtr, uint32_t entityIndex) {
  if (!mapPtr)
    return nullptr;

  auto *provider = static_cast<quakelib::QMapProvider *>(mapPtr);
  auto solidEntities = provider->GetSolidEntities();

  if (entityIndex >= solidEntities.size())
    return nullptr;

  auto &entity = solidEntities[entityIndex];
  auto meshes = provider->GetEntityMeshes(entity);
  auto textureNames = provider->GetTextureNames();

  auto *outMesh = (QLibMapEntityMesh *)QLib_Malloc(sizeof(QLibMapEntityMesh));
  std::memset(outMesh, 0, sizeof(QLibMapEntityMesh));

  SafeStrCopy(outMesh->className, entity->ClassName(), 64);

  // Set bounds (cast to SolidMapEntity to access bounds methods)
  auto mapEntity = std::static_pointer_cast<quakelib::map::SolidMapEntity>(entity);
  auto center = mapEntity->GetCenter();
  auto boundsMin = mapEntity->GetMin();
  auto boundsMax = mapEntity->GetMax();
  outMesh->center = {center.x(), center.y(), center.z()};
  outMesh->boundsMin = {boundsMin.x(), boundsMin.y(), boundsMin.z()};
  outMesh->boundsMax = {boundsMax.x(), boundsMax.y(), boundsMax.z()};

  // Count totals
  uint32_t totalVerts = 0;
  uint32_t totalIndices = 0;
  for (const auto &mesh : meshes) {
    totalVerts += static_cast<uint32_t>(mesh.vertices.size());
    totalIndices += static_cast<uint32_t>(mesh.indices.size());
  }

  outMesh->totalVertexCount = totalVerts;
  outMesh->totalIndexCount = totalIndices;
  outMesh->submeshCount = static_cast<uint32_t>(meshes.size());

  // Allocate
  if (totalVerts > 0)
    outMesh->vertices = (QLibVertex *)QLib_Malloc(totalVerts * sizeof(QLibVertex));
  if (totalIndices > 0)
    outMesh->indices = (uint32_t *)QLib_Malloc(totalIndices * sizeof(uint32_t));
  if (outMesh->submeshCount > 0)
    outMesh->submeshes = (QLibMapSubmesh *)QLib_Malloc(outMesh->submeshCount * sizeof(QLibMapSubmesh));

  // Fill
  uint32_t vertOffset = 0;
  uint32_t idxOffset = 0;

  for (uint32_t i = 0; i < outMesh->submeshCount; i++) {
    const auto &mesh = meshes[i];
    auto &submesh = outMesh->submeshes[i];

    submesh.vertexOffset = vertOffset;
    submesh.vertexCount = static_cast<uint32_t>(mesh.vertices.size());
    submesh.indexOffset = idxOffset;
    submesh.indexCount = static_cast<uint32_t>(mesh.indices.size());
    submesh.textureID = -1;
    SafeStrCopy(submesh.textureName, mesh.textureName, 64);
    submesh.surfaceType = static_cast<uint8_t>(mesh.type);

    // Find texture ID
    for (size_t k = 0; k < textureNames.size(); k++) {
      if (mesh.textureName == textureNames[k]) {
        submesh.textureID = static_cast<int32_t>(k);
        break;
      }
    }

    for (uint32_t j = 0; j < submesh.vertexCount; j++) {
      outMesh->vertices[vertOffset + j] = ToQLibVertex(mesh.vertices[j]);
    }

    for (uint32_t j = 0; j < submesh.indexCount; j++) {
      outMesh->indices[idxOffset + j] = mesh.indices[j] + vertOffset;
    }

    vertOffset += submesh.vertexCount;
    idxOffset += submesh.indexCount;
  }

  // Export attributes
  ExportAttributes(entity.get(), &outMesh->attributeCount, &outMesh->attributeKeys,
                   &outMesh->attributeValues);

  return outMesh;
}

API_EXPORT void QLibMap_SetFaceType(void *mapPtr, const char *textureName, uint8_t surfaceType) {
  if (!mapPtr || !textureName)
    return;

  auto *provider = static_cast<quakelib::QMapProvider *>(mapPtr);
  provider->SetFaceType(textureName, static_cast<quakelib::SurfaceType>(surfaceType));
}

API_EXPORT void QLibMap_FreeMesh(QLibMapEntityMesh *mesh) {
  if (!mesh)
    return;

  if (mesh->vertices)
    QLib_Free(mesh->vertices);
  if (mesh->indices)
    QLib_Free(mesh->indices);
  if (mesh->submeshes)
    QLib_Free(mesh->submeshes);
  FreeStringArray(mesh->attributeKeys, mesh->attributeCount);
  FreeStringArray(mesh->attributeValues, mesh->attributeCount);
  QLib_Free(mesh);
}

API_EXPORT void QLibMap_FreeData(QLibMapData *data) {
  if (!data)
    return;

  FreeStringArray(data->textureNames, data->textureCount);
  FreeStringArray(data->requiredWads, data->requiredWadCount);

  // Free solid entities
  if (data->solidEntities) {
    for (uint32_t i = 0; i < data->solidEntityCount; i++) {
      auto &mesh = data->solidEntities[i];
      if (mesh.vertices)
        QLib_Free(mesh.vertices);
      if (mesh.indices)
        QLib_Free(mesh.indices);
      if (mesh.submeshes)
        QLib_Free(mesh.submeshes);
      FreeStringArray(mesh.attributeKeys, mesh.attributeCount);
      FreeStringArray(mesh.attributeValues, mesh.attributeCount);
    }
    QLib_Free(data->solidEntities);
  }

  // Free point entities
  if (data->pointEntities) {
    for (uint32_t i = 0; i < data->pointEntityCount; i++) {
      auto &ent = data->pointEntities[i];
      FreeStringArray(ent.attributeKeys, ent.attributeCount);
      FreeStringArray(ent.attributeValues, ent.attributeCount);
    }
    QLib_Free(data->pointEntities);
  }

  QLib_Free(data);
}

API_EXPORT void QLibMap_Destroy(void *mapPtr) {
  if (!mapPtr)
    return;

  // Clean up texture sizes map for this provider
  g_providerTextureSizes.erase(mapPtr);

  delete static_cast<quakelib::QMapProvider *>(mapPtr);
}
