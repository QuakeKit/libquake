# BSP System

The BSP (Binary Space Partition) system in libquake provides functionality for loading and processing compiled Quake BSP files. BSP files are the compiled level format used by the Quake engine, containing pre-calculated geometric data, textures, lighting information, and entity definitions.

## Overview

BSP files are automatically generated from `.map` source files and contain all the information needed to render a Quake level efficiently. Unlike the editable map format, BSP files include:

- Pre-calculated BSP tree for spatial partitioning
- Embedded texture data (mip-mapped)
- Pre-computed lightmaps
- Visibility information (PVS - Potentially Visible Set)
- Entity definitions
- Collision geometry (clip nodes)

## BSP File Structure

A BSP file consists of a header followed by multiple "lumps" (data sections). The file format is described in detail in the [Quake Specifications](https://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_4.htm).

### Key Components

**Models**: The level is divided into one or more models. The first model is always the main world geometry (worldspawn). Additional models represent dynamic objects like doors, platforms, and triggers.

**Vertices**: 3D positions stored as floating-point coordinates, used to define the geometry.

**Edges**: Pairs of vertex indices that define the edges of faces.

**Faces**: Convex polygons that make up the visible surfaces. Each face references:
- A plane equation
- A list of edges
- Texture information
- Lightmap data

**Textures (Miptex)**: Embedded texture data with multiple mipmap levels for efficient rendering at different distances. Textures are stored with 4 scales: full, 1/2, 1/4, and 1/8 resolution.

**Planes**: Define the splitting planes used by the BSP tree and the orientation of faces.

**BSP Tree Nodes**: Hierarchical spatial partitioning structure used for efficient rendering and collision detection.

**BSP Leaves**: Terminal nodes of the BSP tree representing convex regions of space. Contains:
- Face lists
- Visibility information
- Ambient sound levels
- Content type (solid, water, slime, lava, sky)

**Lightmaps**: Pre-calculated lighting data sampled every 16 units, providing smooth shading on surfaces.

**Visibility Lists**: Run-length encoded bitarrays indicating which leaves are potentially visible from any given leaf (PVS data).

**Clip Nodes**: Simplified collision geometry for fast physics calculations.

**Entities**: Text-based entity definitions (same format as MAP files) defining gameplay objects, monsters, items, and level properties.

## Usage

### Loading a BSP File

```cpp
#include <quakelib/bsp/qbsp_provider.h>

// Create provider
quakelib::QBspProvider provider;

// Configure loading options
quakelib::bsp::QBspConfig config;
config.loadTextures = true;        // Load texture lump
config.loadTextureData = true;     // Load actual texture pixel data
config.convertCoordToOGL = true;   // Convert to OpenGL coordinate system

// Load the BSP file
if (!provider.Load("maps/e1m1.bsp", config)) {
    // Handle error
}
```

### Accessing Geometry

```cpp
// Get all solid entities (including worldspawn)
auto solidEntities = provider.GetSolidEntities();

// Get render meshes for an entity
for (const auto& entity : solidEntities) {
    auto meshes = provider.GetEntityMeshes(entity);
    
    for (const auto& mesh : meshes) {
        // mesh.textureName - Name of texture
        // mesh.vertices - Vertex data with positions, normals, UVs, lightmap UVs
        // mesh.indices - Triangle indices
        // mesh.type - Surface type (SOLID, CLIP, SKIP, NODRAW)
    }
}
```

### Accessing Entities

```cpp
// Get point entities (lights, spawn points, monsters, etc.)
auto pointEntities = provider.GetPointEntities();

for (const auto& entity : pointEntities) {
    std::string className = entity->ClassName();
    auto origin = entity->Origin();
    float angle = entity->Angle();
    
    // Access custom attributes
    std::string target = entity->AttributeStr("target");
}
```

### Accessing Textures

```cpp
// Get list of all texture names
auto textureNames = provider.GetTextureNames();

// Get texture data for a specific texture
auto texData = provider.GetTextureData("metal1_1");
if (texData) {
    int width = texData->width;
    int height = texData->height;
    const auto& pixels = texData->data; // RGBA format
}
```

### Accessing Lightmaps

```cpp
// Get the packed lightmap atlas
auto lightmapData = provider.GetLightmapData();
if (lightmapData) {
    int width = lightmapData->width;
    int height = lightmapData->height;
    const auto& pixels = lightmapData->data; // RGBA format
}
```

## Configuration Options

### QBspConfig

- **`loadTextures`** (default: `true`): Whether to load the texture lump from the BSP file.
- **`loadTextureData`** (default: `true`): Whether to extract pixel data from textures. Set to false if you only need texture names.
- **`convertCoordToOGL`** (default: `false`): Convert from Quake's coordinate system (X forward, Y left, Z up) to OpenGL's coordinate system (X right, Y up, Z back).

## Special Texture Names

BSP files use special naming conventions for textures:

- **`sky*`**: Sky textures with dual-layer scrolling
- **`*water`**: Animated water texture with swirl effect
- **`*lava`**: Animated lava texture
- **`*slime`**: Animated slime texture
- Textures starting with **`+`** followed by a digit (0-9): Multi-frame animated textures

## Coordinate Systems

Quake uses a different coordinate system than many modern engines:
- **Quake**: X = forward, Y = left, Z = up
- **OpenGL**: X = right, Y = up, Z = backward

Enable `convertCoordToOGL` in the config to automatically convert coordinates when loading.

## Technical Details

### BSP Tree Traversal

The BSP tree efficiently partitions 3D space for rendering. Each node contains:
- A splitting plane
- Front and back child references (node or leaf)
- Bounding box
- Face references

Leaf nodes (bit 15 set) represent convex regions and contain:
- Content type (solid, water, lava, etc.)
- Visibility list index
- Face references
- Ambient sound levels

### Visibility (PVS)

The Potentially Visible Set uses pre-calculated visibility data to cull entire regions of the level that cannot be seen from the current viewpoint. This dramatically reduces rendering overhead in complex levels.

### Lightmaps

Lightmaps store pre-calculated lighting information for each surface. Understanding how they work:

**Sampling Resolution**: Lightmaps use lower resolution than textures to save memory:
```cpp
// For a face that spans 256x128 world units:
int lightmapWidth = 256 / 16;   // = 16 samples
int lightmapHeight = 128 / 16;  // = 8 samples
// Total: 16 x 8 = 128 lighting samples for this face
```

**Smooth Lighting**: The engine interpolates between samples:
```cpp
// Pseudo-code for sampling lightmap at a world position
vec3 GetLightAtPosition(vec3 worldPos) {
    // Convert world position to lightmap coordinates
    vec2 lightmapUV = CalculateLightmapUV(worldPos);
    
    // Sample lightmap with bilinear filtering
    // This smooths the 16-unit steps into gradual lighting
    return BilinearSample(lightmapTexture, lightmapUV);
}
```

**Dynamic Light Styles**: Each face can reference a light style for special effects:
```cpp
// Pseudo-code showing how light styles work
float GetFinalBrightness(Face face, float time) {
    // Get the static lightmap value (0-255)
    float lightmapValue = SampleLightmap(face);
    
    // Apply dynamic light style (flickering, pulsing, etc.)
    float styleMultiplier = GetLightStyle(face.lightStyle, time);
    
    // Combine: brighter base light, darker ambient
    float finalLight = (lightmapValue * styleMultiplier) - face.baseLight;
    
    return clamp(finalLight, 0.0, 255.0);
}
```

Light styles enable effects like:
- **Style 0**: Normal static lighting
- **Style 1**: Fast flickering (damaged lights)
- **Style 2**: Slow pulsing
- **Styles 3-10**: Various custom patterns

## API Reference

::: doxy.libquake.class
  name: quakelib::bsp::QBsp

::: doxy.libquake.class
  name: quakelib::QBspProvider

::: doxy.libquake.struct
  name: quakelib::bsp::QBspConfig

## References

- [Quake Specifications - BSP File Format](https://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_4.htm)
- [Quake Specifications - Map Format](https://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_2.htm)
