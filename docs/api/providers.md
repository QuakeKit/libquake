# Map Provider Interface

The Map Provider interface provides a unified way to load and access Quake level data from different sources (MAP files or BSP files). This abstraction allows applications to work with both editable source maps and compiled BSP files using the same API.

## Overview

The `IMapProvider` interface defines common operations for:
- Loading level data
- Generating renderable geometry
- Accessing entities (solid and point)
- Retrieving texture information
- Obtaining render meshes with vertices and indices

Two implementations are provided:
- **`QMapProvider`**: Loads `.map` source files with CSG operations
- **`QBspProvider`**: Loads compiled `.bsp` files

## Interface

### Loading Data

```cpp
#include <quakelib/map_provider.h>

// Load a level file
virtual bool Load(const std::string &path) = 0;

// Generate geometry (for MAP files, performs CSG; for BSP, already processed)
virtual void GenerateGeometry(bool csg = true) = 0;
```

### Accessing Entities

```cpp
// Get all solid (brush-based) entities
virtual std::vector<SolidEntityPtr> GetSolidEntities() const = 0;

// Get all point entities
virtual std::vector<PointEntityPtr> GetPointEntities() const = 0;
```

### Accessing Geometry

```cpp
// Get renderable meshes for a solid entity
virtual std::vector<RenderMesh> GetEntityMeshes(const SolidEntityPtr &entity) = 0;
```

### Texture Information

```cpp
// Get list of all texture names used in the level
virtual std::vector<std::string> GetTextureNames() const = 0;

// Get required WAD files (MAP only)
virtual std::vector<std::string> GetRequiredWads() const = 0;

// Get texture pixel data
virtual std::optional<TextureData> GetTextureData(const std::string &name) const = 0;

// Get lightmap atlas data
virtual std::optional<TextureData> GetLightmapData() const = 0;
```

### Surface Type Control

```cpp
// Set surface type for textures (e.g., clip, skip)
virtual void SetFaceType(const std::string &textureName, SurfaceType type) = 0;
```

## QMapProvider

Loads and processes `.map` source files.

### Usage

```cpp
#include <quakelib/map/qmap_provider.h>

quakelib::QMapProvider provider;

// Configure loading
quakelib::map::QMapConfig config;
config.csg = true;                  // Enable CSG operations
config.convertCoordToOGL = false;   // OpenGL coordinate conversion

// Load MAP file
if (!provider.Load("maps/mymap.map", config)) {
    // Handle error
}

// Generate geometry (applies CSG if configured)
provider.GenerateGeometry();
```

### Features

- Parses brush and entity definitions
- Performs CSG (Constructive Solid Geometry) operations
- Generates triangulated meshes
- Fixes T-junctions
- Supports texture alignment
- Can convert coordinates to OpenGL system

## QBspProvider

Loads compiled `.bsp` files.

### Usage

```cpp
#include <quakelib/bsp/qbsp_provider.h>

quakelib::QBspProvider provider;

// Configure loading
quakelib::bsp::QBspConfig config;
config.loadTextures = true;
config.loadTextureData = true;
config.convertCoordToOGL = false;

// Load BSP file
if (!provider.Load("maps/e1m1.bsp", config)) {
    // Handle error
}
```

### Features

- Reads BSP tree structure
- Extracts embedded textures
- Loads pre-calculated lightmaps
- Parses entity definitions
- Reconstructs geometry from BSP data
- Optional coordinate system conversion

## Common Workflow

```cpp
#include <quakelib/map_provider.h>

// Use the interface pointer
quakelib::IMapProvider* provider = nullptr;

// Choose implementation based on file extension
if (filePath.ends_with(".bsp")) {
    auto bspProvider = new quakelib::QBspProvider();
    quakelib::bsp::QBspConfig cfg;
    cfg.convertCoordToOGL = true;
    bspProvider->Load(filePath, cfg);
    provider = bspProvider;
} else {
    auto mapProvider = new quakelib::QMapProvider();
    quakelib::map::QMapConfig cfg;
    cfg.csg = true;
    cfg.convertCoordToOGL = true;
    mapProvider->Load(filePath, cfg);
    mapProvider->GenerateGeometry();
    provider = mapProvider;
}

// Work with either provider through common interface
auto solidEntities = provider->GetSolidEntities();
for (const auto& entity : solidEntities) {
    auto meshes = provider->GetEntityMeshes(entity);
    // Render meshes...
}

delete provider;
```

## Data Structures

### RenderMesh

```cpp
struct RenderMesh {
    std::string textureName;           // Texture name
    int textureWidth;                  // Texture dimensions
    int textureHeight;
    std::vector<Vertex> vertices;      // Vertex data
    std::vector<uint32_t> indices;     // Triangle indices
    SurfaceType type;                  // SOLID, CLIP, SKIP, NODRAW
};
```

### Vertex

```cpp
struct Vertex {
    fvec3 point;          // Position
    fvec3 normal;         // Normal vector
    fvec2 uv;             // Texture coordinates
    fvec2 lightmap_uv;    // Lightmap coordinates
    fvec4 tangent;        // Tangent + bitangent sign
};
```

### TextureData

```cpp
struct TextureData {
    int width;
    int height;
    std::vector<unsigned char> data;  // RGBA pixel data
};
```

### SurfaceType

```cpp
enum class SurfaceType {
    SOLID,    // Normal visible surface
    CLIP,     // Collision only, not rendered
    SKIP,     // Ignored/removed
    NODRAW    // No rendering, used for triggers
};
```

## Configuration

Both providers support configuration through their respective config structures, which inherit from the common `Config` base:

### Common Options (Config)

- **`convertCoordToOGL`**: Convert from Quake to OpenGL coordinates

### BSP-Specific (QBspConfig)

- **`loadTextures`**: Load texture lump
- **`loadTextureData`**: Extract pixel data

### MAP-Specific (QMapConfig)

- **`csg`**: Enable CSG operations (brush clipping)

## API Reference

::: doxy.libquake.class
  name: quakelib::IMapProvider

::: doxy.libquake.class
  name: quakelib::QMapProvider

::: doxy.libquake.class
  name: quakelib::QBspProvider

::: doxy.libquake.struct
  name: quakelib::Config

::: doxy.libquake.struct
  name: quakelib::map::QMapConfig

::: doxy.libquake.struct
  name: quakelib::bsp::QBspConfig
