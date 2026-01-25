# C Wrapper API Reference

The libquake C wrapper provides a C-compatible API for accessing Quake file formats (WAD, BSP, and MAP) from any programming language that supports C FFI (Foreign Function Interface). This wrapper is designed for interoperability with languages like C#, Python, Rust, and others.

## Overview

The wrapper exposes three main APIs:
- **WAD API**: For loading and extracting textures from Quake WAD files
- **BSP API**: For loading and processing compiled Quake BSP level files
- **MAP API**: For loading and compiling Quake MAP source files

All structures use C-compatible types and follow a consistent memory management pattern.

## Memory Management

### Allocation Strategy

The wrapper uses two allocation patterns:

1. **Opaque Pointers**: Internal C++ objects are returned as `void*` and must be destroyed with their corresponding destroy function
2. **Exported Data**: Structures with exported data are allocated using `malloc()` and must be freed with their corresponding free function

### Important Rules

- Always call the appropriate free/destroy function for each loaded resource
- Never manually free individual fields within structures unless specified
- String arrays and texture data are owned by the structure and freed automatically

## Common Structures

### QLibVec2

2D floating-point vector.

```c
struct QLibVec2 {
    float x;
    float y;
};
```

**Memory Layout:**
```
+--------+--------+
|   x    |   y    |
| 4 bytes| 4 bytes|
+--------+--------+
Total: 8 bytes
```

### QLibVec3

3D floating-point vector, commonly used for positions and normals.

```c
struct QLibVec3 {
    float x;
    float y;
    float z;
};
```

**Memory Layout:**
```
+--------+--------+--------+
|   x    |   y    |   z    |
| 4 bytes| 4 bytes| 4 bytes|
+--------+--------+--------+
Total: 12 bytes
```

### QLibVec4

4D floating-point vector, typically used for tangents (with w component for handedness).

```c
struct QLibVec4 {
    float x;
    float y;
    float z;
    float w;
};
```

**Memory Layout:**
```
+--------+--------+--------+--------+
|   x    |   y    |   z    |   w    |
| 4 bytes| 4 bytes| 4 bytes| 4 bytes|
+--------+--------+--------+--------+
Total: 16 bytes
```

### QLibVertex

Complete vertex structure with all attributes for rendering.

```c
struct QLibVertex {
    QLibVec3 position;      // 3D position
    QLibVec3 normal;        // Surface normal
    QLibVec4 tangent;       // Tangent vector (w = handedness)
    QLibVec2 uv;           // Texture coordinates
    QLibVec2 lightmapUV;   // Lightmap coordinates
};
```

**Memory Layout:**
```
+-------------------+  Offset 0
|    position       |  12 bytes (QLibVec3)
+-------------------+  Offset 12
|    normal         |  12 bytes (QLibVec3)
+-------------------+  Offset 24
|    tangent        |  16 bytes (QLibVec4)
+-------------------+  Offset 40
|    uv             |  8 bytes (QLibVec2)
+-------------------+  Offset 48
|    lightmapUV     |  8 bytes (QLibVec2)
+-------------------+  Offset 56
Total: 56 bytes
```

---

## WAD API

The WAD API provides functions for loading Quake WAD texture archives and extracting texture data.

### QLibWadTexture

Represents a single texture from a WAD file.

```c
struct QLibWadTexture {
    char name[16];          // Texture name (null-terminated)
    uint32_t width;         // Texture width in pixels
    uint32_t height;        // Texture height in pixels
    uint32_t dataSize;      // Size of RGBA data in bytes
    uint8_t* data;          // RGBA pixel data (4 bytes per pixel)
    uint8_t isSky;          // 1 if sky texture, 0 otherwise
};
```

**Memory Layout:**
```
+-------------------+  Offset 0
|    name[16]       |  16 bytes (fixed array)
+-------------------+  Offset 16
|    width          |  4 bytes (uint32_t)
+-------------------+  Offset 20
|    height         |  4 bytes (uint32_t)
+-------------------+  Offset 24
|    dataSize       |  4 bytes (uint32_t)
+-------------------+  Offset 28
|    data*          |  8 bytes (pointer)
+-------------------+  Offset 36
|    isSky          |  1 byte (uint8_t)
+-------------------+  Offset 37
| (padding)         |  3 bytes (alignment)
+-------------------+  Offset 40
Total: 40 bytes (64-bit platform)
```

### QLibWadData

Container for all textures in a WAD file.

```c
struct QLibWadData {
    uint32_t textureCount;      // Number of textures
    QLibWadTexture* textures;   // Array of textures
};
```

**Memory Layout:**
```
+-------------------+  Offset 0
|  textureCount     |  4 bytes (uint32_t)
+-------------------+  Offset 4
| (padding)         |  4 bytes (alignment)
+-------------------+  Offset 8
|  textures*        |  8 bytes (pointer)
+-------------------+  Offset 16
Total: 16 bytes (64-bit platform)
```

### Functions

#### QLibWad_Load

Load a WAD file and return an opaque handle.

```c
void* QLibWad_Load(const char* filePath, uint8_t flipHorizontal);
```

**Parameters:**
- `filePath`: Path to the WAD file
- `flipHorizontal`: Set to 1 to flip textures horizontally, 0 otherwise

**Returns:** Opaque pointer to WAD data, or NULL on failure

**Example:**
```c
void* wad = QLibWad_Load("textures/medieval.wad", 0);
if (!wad) {
    fprintf(stderr, "Failed to load WAD file\n");
    return;
}
// ... use wad ...
QLibWad_Destroy(wad);
```

#### QLibWad_ExportAll

Export all textures from a loaded WAD file.

```c
QLibWadData* QLibWad_ExportAll(void* wadPtr);
```

**Parameters:**
- `wadPtr`: Opaque WAD handle from QLibWad_Load

**Returns:** Pointer to QLibWadData structure, or NULL on failure

**Example:**
```c
QLibWadData* data = QLibWad_ExportAll(wad);
if (data) {
    for (uint32_t i = 0; i < data->textureCount; i++) {
        QLibWadTexture* tex = &data->textures[i];
        printf("Texture: %s (%dx%d)\n", tex->name, tex->width, tex->height);
        
        // Upload to GPU or process RGBA data
        // tex->data contains width * height * 4 bytes
    }
    QLibWad_FreeData(data);
}
```

#### QLibWad_GetTexture

Get a specific texture by name.

```c
QLibWadTexture* QLibWad_GetTexture(void* wadPtr, const char* name);
```

**Parameters:**
- `wadPtr`: Opaque WAD handle from QLibWad_Load
- `name`: Texture name to retrieve

**Returns:** Pointer to QLibWadTexture, or NULL if not found

**Example:**
```c
QLibWadTexture* tex = QLibWad_GetTexture(wad, "METAL1");
if (tex) {
    printf("Found texture: %dx%d, isSky=%d\n", tex->width, tex->height, tex->isSky);
    QLibWad_FreeTexture(tex);
}
```

#### QLibWad_FreeTexture

Free a single texture obtained from QLibWad_GetTexture.

```c
void QLibWad_FreeTexture(QLibWadTexture* texture);
```

#### QLibWad_FreeData

Free exported WAD data from QLibWad_ExportAll.

```c
void QLibWad_FreeData(QLibWadData* data);
```

#### QLibWad_Destroy

Destroy the WAD handle and release all resources.

```c
void QLibWad_Destroy(void* wadPtr);
```

### Complete WAD Example

```c
#include "wrapper.h"
#include <stdio.h>

void process_wad(const char* filepath) {
    // Load WAD file
    void* wad = QLibWad_Load(filepath, 0);
    if (!wad) {
        fprintf(stderr, "Failed to load %s\n", filepath);
        return;
    }
    
    // Export all textures
    QLibWadData* data = QLibWad_ExportAll(wad);
    if (!data) {
        fprintf(stderr, "Failed to export textures\n");
        QLibWad_Destroy(wad);
        return;
    }
    
    printf("WAD contains %u textures:\n", data->textureCount);
    
    // Process each texture
    for (uint32_t i = 0; i < data->textureCount; i++) {
        QLibWadTexture* tex = &data->textures[i];
        printf("  [%u] %s: %ux%u pixels (%u bytes)%s\n",
               i, tex->name, tex->width, tex->height, tex->dataSize,
               tex->isSky ? " [SKY]" : "");
        
        // Access RGBA data
        // Each pixel is 4 bytes: R, G, B, A
        // Total size: width * height * 4 = dataSize
        uint8_t* pixels = tex->data;
        // ... upload to GPU, save to file, etc ...
    }
    
    // Clean up
    QLibWad_FreeData(data);
    QLibWad_Destroy(wad);
}
```

---

## BSP API

The BSP API provides functions for loading compiled Quake BSP level files, including geometry, textures, and entities.

### QLibBspTexture

Texture information from a BSP file.

```c
struct QLibBspTexture {
    char name[64];          // Texture name
    uint32_t width;         // Width in pixels
    uint32_t height;        // Height in pixels
    uint32_t dataSize;      // Size of RGBA data (may be 0)
    uint8_t* data;          // RGBA pixel data (may be NULL)
};
```

**Memory Layout:**
```
+-------------------+  Offset 0
|    name[64]       |  64 bytes
+-------------------+  Offset 64
|    width          |  4 bytes
+-------------------+  Offset 68
|    height         |  4 bytes
+-------------------+  Offset 72
|    dataSize       |  4 bytes
+-------------------+  Offset 76
| (padding)         |  4 bytes
+-------------------+  Offset 80
|    data*          |  8 bytes
+-------------------+  Offset 88
Total: 88 bytes
```

### QLibBspSubmesh

Represents a renderable submesh with a single texture.

```c
struct QLibBspSubmesh {
    uint32_t vertexOffset;      // Offset into vertex array
    uint32_t vertexCount;       // Number of vertices
    uint32_t indexOffset;       // Offset into index array
    uint32_t indexCount;        // Number of indices
    int32_t textureIndex;       // Index into texture array
    char textureName[64];       // Texture name
};
```

**Memory Layout:**
```
+-------------------+  Offset 0
|  vertexOffset     |  4 bytes
+-------------------+  Offset 4
|  vertexCount      |  4 bytes
+-------------------+  Offset 8
|  indexOffset      |  4 bytes
+-------------------+  Offset 12
|  indexCount       |  4 bytes
+-------------------+  Offset 16
|  textureIndex     |  4 bytes
+-------------------+  Offset 20
| (padding)         |  4 bytes
+-------------------+  Offset 24
|  textureName[64]  |  64 bytes
+-------------------+  Offset 88
Total: 88 bytes
```

### QLibBspEntityMesh

Complete mesh data for a BSP entity (worldspawn or func_* entities).

```c
struct QLibBspEntityMesh {
    char className[64];         // Entity class name
    QLibVec3 center;            // Bounding box center
    QLibVec3 boundsMin;         // Minimum bounds
    QLibVec3 boundsMax;         // Maximum bounds
    
    uint32_t totalVertexCount;  // Total vertices
    uint32_t totalIndexCount;   // Total indices
    uint32_t submeshCount;      // Number of submeshes
    
    QLibVertex* vertices;       // Vertex array
    uint32_t* indices;          // Index array
    QLibBspSubmesh* submeshes;  // Submesh array
};
```

**Memory Layout:**
```
+-------------------+  Offset 0
|  className[64]    |  64 bytes
+-------------------+  Offset 64
|  center           |  12 bytes (QLibVec3)
+-------------------+  Offset 76
|  boundsMin        |  12 bytes (QLibVec3)
+-------------------+  Offset 88
|  boundsMax        |  12 bytes (QLibVec3)
+-------------------+  Offset 100
| totalVertexCount  |  4 bytes
+-------------------+  Offset 104
| totalIndexCount   |  4 bytes
+-------------------+  Offset 108
| submeshCount      |  4 bytes
+-------------------+  Offset 112
| (padding)         |  4 bytes
+-------------------+  Offset 116
|  vertices*        |  8 bytes
+-------------------+  Offset 124
|  indices*         |  8 bytes
+-------------------+  Offset 132
|  submeshes*       |  8 bytes
+-------------------+  Offset 140
Total: 140 bytes
```

### QLibBspPointEntity

Non-solid entity with position data (lights, spawn points, etc.).

```c
struct QLibBspPointEntity {
    char className[64];         // Entity class name
    QLibVec3 origin;            // Position in world space
    float angle;                // Rotation angle
    uint32_t attributeCount;    // Number of key-value pairs
    char** attributeKeys;       // Array of key strings
    char** attributeValues;     // Array of value strings
};
```

**Memory Layout:**
```
+-------------------+  Offset 0
|  className[64]    |  64 bytes
+-------------------+  Offset 64
|  origin           |  12 bytes (QLibVec3)
+-------------------+  Offset 76
|  angle            |  4 bytes
+-------------------+  Offset 80
|  attributeCount   |  4 bytes
+-------------------+  Offset 84
| (padding)         |  4 bytes
+-------------------+  Offset 88
| attributeKeys*    |  8 bytes
+-------------------+  Offset 96
| attributeValues*  |  8 bytes
+-------------------+  Offset 104
Total: 104 bytes
```

### QLibBspData

Complete BSP file data export.

```c
struct QLibBspData {
    uint32_t version;               // BSP version
    uint32_t textureCount;          // Number of textures
    uint32_t solidEntityCount;      // Number of solid entities
    uint32_t pointEntityCount;      // Number of point entities
    
    QLibBspTexture* textures;       // Texture array
    QLibBspEntityMesh* solidEntities;   // Solid entity meshes
    QLibBspPointEntity* pointEntities;  // Point entities
    
    // Lightmap (optional)
    uint32_t lightmapWidth;         // Lightmap atlas width
    uint32_t lightmapHeight;        // Lightmap atlas height
    uint8_t* lightmapData;          // RGBA lightmap data
};
```

### Functions

#### QLibBsp_Load

Load a BSP file.

```c
void* QLibBsp_Load(const char* filePath, 
                   uint8_t loadTextures, 
                   uint8_t loadTextureData,
                   uint8_t convertToOpenGL);
```

**Parameters:**
- `filePath`: Path to BSP file
- `loadTextures`: Set to 1 to load texture info
- `loadTextureData`: Set to 1 to load texture pixel data
- `convertToOpenGL`: Set to 1 to flip Y-axis for OpenGL

**Returns:** Opaque BSP handle, or NULL on failure

#### QLibBsp_ExportAll

Export all BSP data.

```c
QLibBspData* QLibBsp_ExportAll(void* bspPtr);
```

**Example:**
```c
void* bsp = QLibBsp_Load("maps/e1m1.bsp", 1, 1, 1);
if (!bsp) return;

QLibBspData* data = QLibBsp_ExportAll(bsp);
if (data) {
    printf("BSP Version: %u\n", data->version);
    printf("Textures: %u\n", data->textureCount);
    printf("Solid Entities: %u\n", data->solidEntityCount);
    printf("Point Entities: %u\n", data->pointEntityCount);
    
    // Process worldspawn
    if (data->solidEntityCount > 0) {
        QLibBspEntityMesh* world = &data->solidEntities[0];
        printf("Worldspawn: %u vertices, %u indices, %u submeshes\n",
               world->totalVertexCount, world->totalIndexCount, world->submeshCount);
        
        // Render each submesh
        for (uint32_t i = 0; i < world->submeshCount; i++) {
            QLibBspSubmesh* submesh = &world->submeshes[i];
            printf("  Submesh %u: %s (%u verts, %u indices)\n",
                   i, submesh->textureName, submesh->vertexCount, submesh->indexCount);
            
            // Get vertices for this submesh
            QLibVertex* verts = &world->vertices[submesh->vertexOffset];
            uint32_t* indices = &world->indices[submesh->indexOffset];
            
            // Upload to GPU...
        }
    }
    
    QLibBsp_FreeData(data);
}
QLibBsp_Destroy(bsp);
```

#### QLibBsp_GetEntityMesh

Get a specific entity mesh by index.

```c
QLibBspEntityMesh* QLibBsp_GetEntityMesh(void* bspPtr, uint32_t entityIndex);
```

#### QLibBsp_FreeMesh

Free a single entity mesh.

```c
void QLibBsp_FreeMesh(QLibBspEntityMesh* mesh);
```

#### QLibBsp_FreeData

Free complete BSP data.

```c
void QLibBsp_FreeData(QLibBspData* data);
```

#### QLibBsp_Destroy

Destroy BSP handle.

```c
void QLibBsp_Destroy(void* bspPtr);
```

---

## MAP API

The MAP API provides functions for loading and processing Quake MAP source files with CSG and geometry generation.

### QLibMapSubmesh

Map submesh with surface type information.

```c
struct QLibMapSubmesh {
    uint32_t vertexOffset;
    uint32_t vertexCount;
    uint32_t indexOffset;
    uint32_t indexCount;
    int32_t textureID;
    char textureName[64];
    uint8_t surfaceType;    // 0=SOLID, 1=CLIP, 2=SKIP, 3=NODRAW
};
```

**Memory Layout:** Similar to QLibBspSubmesh with additional `surfaceType` field (add 1 byte + 7 padding)

### QLibMapEntityMesh

Complete mesh for a MAP entity with attributes.

```c
struct QLibMapEntityMesh {
    char className[64];
    QLibVec3 center;
    QLibVec3 boundsMin;
    QLibVec3 boundsMax;
    
    uint32_t totalVertexCount;
    uint32_t totalIndexCount;
    uint32_t submeshCount;
    
    QLibVertex* vertices;
    uint32_t* indices;
    QLibMapSubmesh* submeshes;
    
    uint32_t attributeCount;
    char** attributeKeys;
    char** attributeValues;
};
```

### QLibMapPointEntity

Point entity from MAP file.

```c
struct QLibMapPointEntity {
    char className[64];
    QLibVec3 origin;
    float angle;
    uint32_t attributeCount;
    char** attributeKeys;
    char** attributeValues;
};
```

### QLibMapData

Complete MAP file data.

```c
struct QLibMapData {
    uint32_t solidEntityCount;
    uint32_t pointEntityCount;
    uint32_t textureCount;
    
    QLibMapEntityMesh* solidEntities;
    QLibMapPointEntity* pointEntities;
    char** textureNames;
    char** requiredWads;
    uint32_t requiredWadCount;
};
```

### Functions

#### QLibMap_Load

Load and process a MAP file.

```c
void* QLibMap_Load(const char* filePath, 
                   uint8_t enableCSG, 
                   uint8_t convertToOpenGL);
```

**Parameters:**
- `filePath`: Path to MAP file
- `enableCSG`: Set to 1 to enable CSG (brush clipping)
- `convertToOpenGL`: Set to 1 to flip Y-axis for OpenGL

**Returns:** Opaque MAP handle, or NULL on failure

**Example:**
```c
void* map = QLibMap_Load("maps/test.map", 1, 1);
if (!map) {
    fprintf(stderr, "Failed to load MAP file\n");
    return;
}

QLibMapData* data = QLibMap_ExportAll(map);
if (data) {
    printf("Required WADs: %u\n", data->requiredWadCount);
    for (uint32_t i = 0; i < data->requiredWadCount; i++) {
        printf("  - %s\n", data->requiredWads[i]);
    }
    
    printf("Textures used: %u\n", data->textureCount);
    for (uint32_t i = 0; i < data->textureCount; i++) {
        printf("  - %s\n", data->textureNames[i]);
    }
    
    QLibMap_FreeData(data);
}
QLibMap_Destroy(map);
```

#### QLibMap_ExportAll

Export all MAP data.

```c
QLibMapData* QLibMap_ExportAll(void* mapPtr);
```

#### QLibMap_GetEntityMesh

Get specific entity mesh.

```c
QLibMapEntityMesh* QLibMap_GetEntityMesh(void* mapPtr, uint32_t entityIndex);
```

#### QLibMap_SetFaceType

Set surface type for faces with specific texture.

```c
void QLibMap_SetFaceType(void* mapPtr, const char* textureName, uint8_t surfaceType);
```

**Surface Types:**
- `0` = SOLID (rendered)
- `1` = CLIP (collision only)
- `2` = SKIP (ignored)
- `3` = NODRAW (not rendered)

**Example:**
```c
// Mark all "clip" textures as collision-only
QLibMap_SetFaceType(map, "clip", 1);

// Skip "trigger" textures
QLibMap_SetFaceType(map, "trigger", 2);
```

#### QLibMap_FreeMesh

Free single entity mesh.

```c
void QLibMap_FreeMesh(QLibMapEntityMesh* mesh);
```

#### QLibMap_FreeData

Free complete MAP data.

```c
void QLibMap_FreeData(QLibMapData* data);
```

#### QLibMap_Destroy

Destroy MAP handle.

```c
void QLibMap_Destroy(void* mapPtr);
```

### Complete MAP Example

```c
#include "wrapper.h"
#include <stdio.h>

void load_and_render_map(const char* mapPath) {
    // Load MAP with CSG enabled and OpenGL coordinate system
    void* map = QLibMap_Load(mapPath, 1, 1);
    if (!map) {
        fprintf(stderr, "Failed to load %s\n", mapPath);
        return;
    }
    
    // Configure surface types
    QLibMap_SetFaceType(map, "clip", 1);      // Collision only
    QLibMap_SetFaceType(map, "trigger", 2);   // Skip
    QLibMap_SetFaceType(map, "sky", 3);       // No draw
    
    // Export all data
    QLibMapData* data = QLibMap_ExportAll(map);
    if (!data) {
        QLibMap_Destroy(map);
        return;
    }
    
    printf("=== MAP Data ===\n");
    printf("Solid Entities: %u\n", data->solidEntityCount);
    printf("Point Entities: %u\n", data->pointEntityCount);
    printf("Unique Textures: %u\n", data->textureCount);
    printf("Required WADs: %u\n", data->requiredWadCount);
    
    // List required WAD files
    for (uint32_t i = 0; i < data->requiredWadCount; i++) {
        printf("  WAD: %s\n", data->requiredWads[i]);
    }
    
    // Process worldspawn (first solid entity)
    if (data->solidEntityCount > 0) {
        QLibMapEntityMesh* world = &data->solidEntities[0];
        printf("\nWorldspawn:\n");
        printf("  Class: %s\n", world->className);
        printf("  Center: (%.2f, %.2f, %.2f)\n", 
               world->center.x, world->center.y, world->center.z);
        printf("  Bounds: (%.2f, %.2f, %.2f) to (%.2f, %.2f, %.2f)\n",
               world->boundsMin.x, world->boundsMin.y, world->boundsMin.z,
               world->boundsMax.x, world->boundsMax.y, world->boundsMax.z);
        printf("  Vertices: %u\n", world->totalVertexCount);
        printf("  Indices: %u\n", world->totalIndexCount);
        printf("  Submeshes: %u\n", world->submeshCount);
        
        // List entity attributes
        printf("  Attributes:\n");
        for (uint32_t i = 0; i < world->attributeCount; i++) {
            printf("    %s = %s\n", world->attributeKeys[i], world->attributeValues[i]);
        }
        
        // Process each submesh
        for (uint32_t i = 0; i < world->submeshCount; i++) {
            QLibMapSubmesh* sub = &world->submeshes[i];
            
            const char* typeStr[] = {"SOLID", "CLIP", "SKIP", "NODRAW"};
            printf("\n  Submesh %u:\n", i);
            printf("    Texture: %s\n", sub->textureName);
            printf("    Type: %s\n", typeStr[sub->surfaceType]);
            printf("    Vertices: %u (offset %u)\n", sub->vertexCount, sub->vertexOffset);
            printf("    Indices: %u (offset %u)\n", sub->indexCount, sub->indexOffset);
            
            // Skip non-renderable surfaces
            if (sub->surfaceType != 0) {
                continue;
            }
            
            // Upload to GPU
            QLibVertex* verts = &world->vertices[sub->vertexOffset];
            uint32_t* indices = &world->indices[sub->indexOffset];
            
            // Example: print first vertex
            if (sub->vertexCount > 0) {
                printf("    First vertex: pos(%.2f, %.2f, %.2f) uv(%.2f, %.2f)\n",
                       verts[0].position.x, verts[0].position.y, verts[0].position.z,
                       verts[0].uv.x, verts[0].uv.y);
            }
        }
    }
    
    // Process func_* entities
    for (uint32_t i = 1; i < data->solidEntityCount; i++) {
        QLibMapEntityMesh* ent = &data->solidEntities[i];
        printf("\nEntity %u: %s\n", i, ent->className);
        printf("  Submeshes: %u\n", ent->submeshCount);
        // ... process entity mesh ...
    }
    
    // Process point entities (lights, spawns, etc.)
    for (uint32_t i = 0; i < data->pointEntityCount; i++) {
        QLibMapPointEntity* ent = &data->pointEntities[i];
        printf("\nPoint Entity %u: %s\n", i, ent->className);
        printf("  Origin: (%.2f, %.2f, %.2f)\n", 
               ent->origin.x, ent->origin.y, ent->origin.z);
        printf("  Angle: %.2f\n", ent->angle);
        
        // List attributes
        for (uint32_t j = 0; j < ent->attributeCount; j++) {
            printf("  %s = %s\n", ent->attributeKeys[j], ent->attributeValues[j]);
        }
    }
    
    // Cleanup
    QLibMap_FreeData(data);
    QLibMap_Destroy(map);
}
```

---

## Platform-Specific Notes

### Windows

- Structures are exported with `__declspec(dllexport)`
- Use the generated `.lib` file for linking
- The DLL must be in the same directory as the executable or in PATH

### Linux

- Structures are exported with default visibility
- Link with `-lquake`
- Ensure the `.so` file is in LD_LIBRARY_PATH or system library paths

### macOS

- Structures are exported with `__attribute__((visibility("default")))`
- Link with `-lquake`
- Ensure the `.dylib` file is in DYLD_LIBRARY_PATH or use @rpath

---

## Memory Alignment

All structures follow natural alignment rules for their platform:

- **32-bit platforms**: Pointers are 4 bytes, structures aligned to 4-byte boundaries
- **64-bit platforms**: Pointers are 8 bytes, structures aligned to 8-byte boundaries

When interfacing from other languages:
- Use `#[repr(C)]` in Rust
- Use `[StructLayout(LayoutKind.Sequential)]` in C#
- Use `ctypes.Structure` in Python

---

## Error Handling

Functions that return pointers return `NULL` on failure. Always check return values:

```c
void* resource = QLibWad_Load("texture.wad", 0);
if (!resource) {
    // Handle error - file not found, invalid format, etc.
    return;
}
// ... use resource ...
QLibWad_Destroy(resource);
```

---

## Threading

The wrapper is **not thread-safe**. Each handle (WAD, BSP, MAP) should only be accessed from a single thread. To use the library in a multi-threaded context:

1. Load resources from a single thread
2. Export data using the `ExportAll` functions
3. Share the exported data structures across threads
4. Free resources from the same thread that loaded them

---

## Best Practices

1. **Always free resources**: Call the appropriate destroy/free functions
2. **Check for NULL**: Always validate return values before use
3. **Copy data if needed**: Exported data is only valid until the free function is called
4. **Process submeshes**: Use submesh offsets to access the correct portions of vertex/index arrays
5. **Respect surface types**: Skip rendering CLIP, SKIP, and NODRAW faces in MAP files
6. **Load WADs first**: When loading MAP files, ensure referenced WAD files are available for textures

---

## See Also

- [BSP Format Documentation](../bsp/bsp_format.md)
- [WAD Format Documentation](../wad/wad_format.md)
- [Entity Parsing](../common/entity_parsing.md)
- [Map Provider API](providers.md)
