#pragma once

#include <cstdint>
#include <cstdlib>

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#if defined(_WIN32)
#define API_EXPORT EXTERNC __declspec(dllexport)
#elif defined(__linux__)
#define API_EXPORT EXTERNC
#elif defined(__APPLE__)
#define API_EXPORT EXTERNC __attribute__((visibility("default")))
#endif

#define QLib_Malloc(size) malloc(size)
#define QLib_Free(ptr) free(ptr)

// ============================================================================
// Common Structures
// ============================================================================

struct QLibVec2 {
  float x, y;
};

struct QLibVec3 {
  float x, y, z;
};

struct QLibVec4 {
  float x, y, z, w;
};

struct QLibVertex {
  QLibVec3 position;
  QLibVec3 normal;
  QLibVec4 tangent;
  QLibVec2 uv;
  QLibVec2 lightmapUV;
};

// ============================================================================
// WAD API
// ============================================================================

struct QLibWadTexture {
  char name[16];
  uint32_t width;
  uint32_t height;
  uint32_t dataSize; // Size of RGBA data
  uint8_t *data;     // RGBA pixel data
  uint8_t isSky;     // 0 or 1
};

struct QLibWadData {
  uint32_t textureCount;
  QLibWadTexture *textures;
};

API_EXPORT void *QLibWad_Load(const char *filePath, uint8_t flipHorizontal);
API_EXPORT QLibWadData *QLibWad_ExportAll(void *wadPtr);
API_EXPORT QLibWadTexture *QLibWad_GetTexture(void *wadPtr, const char *name);
API_EXPORT void QLibWad_FreeTexture(QLibWadTexture *texture);
API_EXPORT void QLibWad_FreeData(QLibWadData *data);
API_EXPORT void QLibWad_Destroy(void *wadPtr);

// ============================================================================
// BSP API
// ============================================================================

struct QLibBspTexture {
  char name[64];
  uint32_t width;
  uint32_t height;
  uint32_t dataSize;
  uint8_t *data; // RGBA pixel data (may be NULL if no data)
};

struct QLibBspSubmesh {
  uint32_t vertexOffset;
  uint32_t vertexCount;
  uint32_t indexOffset;
  uint32_t indexCount;
  int32_t textureIndex;
  char textureName[64];
};

struct QLibBspEntityMesh {
  char className[64];
  QLibVec3 center;
  QLibVec3 boundsMin;
  QLibVec3 boundsMax;

  uint32_t totalVertexCount;
  uint32_t totalIndexCount;
  uint32_t submeshCount;

  QLibVertex *vertices;
  uint32_t *indices;
  QLibBspSubmesh *submeshes;
};

struct QLibBspPointEntity {
  char className[64];
  QLibVec3 origin;
  float angle;
  uint32_t attributeCount;
  char **attributeKeys;
  char **attributeValues;
};

struct QLibBspData {
  uint32_t version;
  uint32_t textureCount;
  uint32_t solidEntityCount;
  uint32_t pointEntityCount;

  QLibBspTexture *textures;
  QLibBspEntityMesh *solidEntities;
  QLibBspPointEntity *pointEntities;

  // Lightmap data (optional)
  uint32_t lightmapWidth;
  uint32_t lightmapHeight;
  uint8_t *lightmapData; // RGBA
};

API_EXPORT void *QLibBsp_Load(const char *filePath, uint8_t loadTextures, uint8_t loadTextureData,
                              uint8_t convertToOpenGL);
API_EXPORT QLibBspData *QLibBsp_ExportAll(void *bspPtr);
API_EXPORT QLibBspEntityMesh *QLibBsp_GetEntityMesh(void *bspPtr, uint32_t entityIndex);
API_EXPORT void QLibBsp_FreeMesh(QLibBspEntityMesh *mesh);
API_EXPORT void QLibBsp_FreeData(QLibBspData *data);
API_EXPORT void QLibBsp_Destroy(void *bspPtr);

// ============================================================================
// MAP API
// ============================================================================

struct QLibMapSubmesh {
  uint32_t vertexOffset;
  uint32_t vertexCount;
  uint32_t indexOffset;
  uint32_t indexCount;
  int32_t textureID;
  char textureName[64];
  uint8_t surfaceType; // 0=SOLID, 1=CLIP, 2=SKIP, 3=NODRAW
};

struct QLibMapEntityMesh {
  char className[64];
  QLibVec3 center;
  QLibVec3 boundsMin;
  QLibVec3 boundsMax;

  uint32_t totalVertexCount;
  uint32_t totalIndexCount;
  uint32_t submeshCount;

  QLibVertex *vertices;
  uint32_t *indices;
  QLibMapSubmesh *submeshes;

  uint32_t attributeCount;
  char **attributeKeys;
  char **attributeValues;
};

struct QLibMapPointEntity {
  char className[64];
  QLibVec3 origin;
  float angle;
  uint32_t attributeCount;
  char **attributeKeys;
  char **attributeValues;
};

struct QLibMapData {
  uint32_t solidEntityCount;
  uint32_t pointEntityCount;
  uint32_t textureCount;

  QLibMapEntityMesh *solidEntities;
  QLibMapPointEntity *pointEntities;
  char **textureNames;
  char **requiredWads;
  uint32_t requiredWadCount;
};

API_EXPORT void *QLibMap_Load(const char *filePath, uint8_t enableCSG, uint8_t convertToOpenGL);
API_EXPORT char **QLibMap_GetRequiredWads(void *mapPtr, uint32_t *outCount);
API_EXPORT char **QLibMap_GetTextureNames(void *mapPtr, uint32_t *outCount);
API_EXPORT void QLibMap_RegisterTextureSize(void *mapPtr, const char *textureName, uint32_t width,
                                            uint32_t height);
API_EXPORT void QLibMap_GenerateGeometry(void *mapPtr);
API_EXPORT QLibMapData *QLibMap_ExportAll(void *mapPtr);
API_EXPORT QLibMapEntityMesh *QLibMap_GetEntityMesh(void *mapPtr, uint32_t entityIndex);
API_EXPORT void QLibMap_SetFaceType(void *mapPtr, const char *textureName, uint8_t surfaceType);
API_EXPORT void QLibMap_FreeMesh(QLibMapEntityMesh *mesh);
API_EXPORT void QLibMap_FreeData(QLibMapData *data);
API_EXPORT void QLibMap_Destroy(void *mapPtr);
