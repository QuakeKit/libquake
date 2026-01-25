# BSP File Format

This document describes the BSP (Binary Space Partition) file format used by Quake to store compiled level data.

## Overview

BSP files are the compiled output of Quake's level compilation process. Unlike the editable `.map` format, BSP files contain pre-calculated data optimized for fast rendering and efficient collision detection.

**Reference**: [Quake Specifications - BSP File Format](https://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_4.htm)

## Purpose

BSP files serve multiple purposes:
- **Fast Rendering**: Pre-calculated BSP tree enables efficient front-to-back rendering
- **Visibility Culling**: PVS (Potentially Visible Set) data dramatically reduces overdraw
- **Collision Detection**: Simplified clip nodes provide fast collision checks
- **Self-Contained**: All textures, lighting, and geometry embedded in one file

## File Structure

A BSP file consists of a header followed by data "lumps":

```
[Header]
  - Version (0x1D for Quake, typically 29 or 30)
  - 15 Directory Entries (lumps)

[Lump 0: Entities] - Text-based entity definitions
[Lump 1: Planes] - Plane equations for splitting and faces
[Lump 2: Textures] - Mip-mapped texture data
[Lump 3: Vertices] - 3D vertex positions
[Lump 4: Visibility] - Compressed PVS data
[Lump 5: Nodes] - BSP tree inner nodes
[Lump 6: Texture Info] - Texture mapping parameters
[Lump 7: Faces] - Surface definitions
[Lump 8: Lighting] - Lightmap data
[Lump 9: Clip Nodes] - Collision tree
[Lump 10: Leaves] - BSP tree leaf nodes
[Lump 11: Face List] - Face indices for leaves
[Lump 12: Edges] - Edge vertex pairs
[Lump 13: Edge List] - Signed edge indices for faces
[Lump 14: Models] - Submodel definitions
```

Each directory entry specifies:
- **offset**: Byte offset from file start
- **size**: Size of lump data in bytes

## Key Concepts

### Binary Space Partition (BSP) Tree

The BSP tree recursively divides 3D space using planes:
- **Nodes**: Interior nodes containing splitting planes
- **Leaves**: Terminal nodes representing convex regions

This structure enables:
- Efficient depth sorting for rendering
- Fast point-in-space queries
- Hierarchical culling

### Models

The level is divided into **models**:
- **Model 0**: The main world geometry (worldspawn)
- **Model 1+**: Dynamic objects (doors, platforms, etc.)

Each model has its own:
- BSP tree for rendering
- Clip nodes for collision
- Face list
- Bounding box

### Coordinate System

Quake uses a **right-handed** coordinate system:
- **X**: Forward (toward positive X)
- **Y**: Left (toward positive Y)
- **Z**: Up (toward positive Z)

This differs from OpenGL (X=right, Y=up, Z=back). Use `convertCoordToOGL` config option to automatically convert.

### Planes

Planes are defined by:
- **normal**: Unit vector perpendicular to plane (nx, ny, nz)
- **dist**: Distance from origin along normal
- **type**: Optimization hint based on normal direction

Plane equation: `dot(point, normal) - dist = 0`

Points with positive result are "in front", negative are "behind".

### Faces

Faces are convex polygons defined by:
- Reference to a splitting plane
- Side of plane (front or back)
- List of edges (via edge list)
- Texture info reference
- Lightmap offset
- Lighting parameters

### Edges and Edge Lists

**Edges**: Simple vertex pairs `(v0, v1)`

**Edge List**: Signed indices to edges:
- Positive index: Use edge normally (v0 → v1)
- Negative index: Use edge reversed (v1 → v0)

This allows edge reuse while maintaining consistent winding order.

### Textures (Miptex)

Textures are stored with multiple mipmap levels:
- **Full resolution** (1:1)
- **Half resolution** (1:2)
- **Quarter resolution** (1:4)
- **Eighth resolution** (1:8)

This reduces aliasing and improves performance at distance.

Special texture names:
- **`sky*`**: Sky with dual-layer scrolling
- **`*water`**, **`*lava`**, **`*slime`**: Animated liquids
- **`+0`** through **`+9`**: Multi-frame animations

### Lightmaps

Lightmaps are pre-calculated lighting stored as grayscale or RGB data. They make surfaces look properly lit without expensive real-time calculations.

#### How Lightmap Resolution Works

Lightmaps use **1/16th** the resolution of world space:

```cpp
// Example: A face with world-space bounds
struct Face {
    vec3 minBounds = {0, 0, 0};
    vec3 maxBounds = {256, 128, 0};  // 256 units wide, 128 units tall
};

// Calculate lightmap size
vec3 extent = maxBounds - minBounds;  // {256, 128, 0}
int lightmapWidth = (int)(extent.x / 16);   // 256/16 = 16 pixels
int lightmapHeight = (int)(extent.y / 16);  // 128/16 = 8 pixels

// Total lightmap size for this face: 16x8 = 128 bytes
```

#### Mapping World Position to Lightmap

To light a pixel, convert its world position to lightmap coordinates:

```cpp
// Get lighting at a specific world position on a face
vec3 GetLightAtWorldPos(vec3 worldPos, Face face) {
    // 1. Project world position onto face's 2D plane
    //    (using the face's texture mapping vectors)
    vec2 faceUV = ProjectToFacePlane(worldPos, face);
    
    // 2. Convert to lightmap UV (0.0 to 1.0 range)
    vec2 lightmapUV;
    lightmapUV.x = (faceUV.x - face.minBounds.x) / (face.maxBounds.x - face.minBounds.x);
    lightmapUV.y = (faceUV.y - face.minBounds.y) / (face.maxBounds.y - face.minBounds.y);
    
    // 3. Sample from the lightmap with bilinear filtering for smooth gradients
    return SampleLightmapBilinear(face.lightmapData, lightmapUV, 
                                   face.lightmapWidth, face.lightmapHeight);
}
```

#### Light Styles (Animated Lighting)

Light styles create flickering, pulsing, and other dynamic effects:

```cpp
// Light style patterns (defined by engine)
const char* lightStyles[] = {
    "m",           // Style 0: Normal (constant 'm' = medium brightness)
    "mmnmmommommnonmmonqnmmo",  // Style 1: Flicker
    "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba",  // Style 2: Slow pulse
    // ... more patterns
};

// Calculate final brightness at runtime
uint8_t CalculateFinalLight(Face face, vec3 worldPos, float gameTime) {
    // 1. Get base lighting from lightmap
    uint8_t lightmapValue = GetLightAtWorldPos(worldPos, face);
    
    // 2. Get current multiplier from light style animation
    const char* pattern = lightStyles[face.lightStyle];
    int patternLength = strlen(pattern);
    int index = (int)(gameTime * 10.0f) % patternLength;  // Animate at 10 Hz
    
    // Convert pattern character to multiplier: 'a'=0.0, 'm'=0.5, 'z'=1.0
    float styleMultiplier = (pattern[index] - 'a') / 25.0f;
    
    // 3. Apply style and base light
    float result = (lightmapValue * styleMultiplier) - face.baseLight;
    
    return (uint8_t)clamp(result, 0.0f, 255.0f);
}
```

**Common Light Styles:**
- **0**: Static light (no animation)
- **1**: Flickering light (damaged/failing)
- **2**: Slow strong pulse
- **3**: Candle flicker
- **10**: Fluorescent flicker

### Visibility (PVS)

The Potentially Visible Set is a run-length encoded bit array:
- One bit per leaf
- Indicates which leaves are potentially visible from each leaf
- Dramatically reduces rendering workload

Decompression:
```
for each byte in vislist:
  if byte == 0:
    skip next_byte * 8 leaves (all invisible)
  else:
    test each bit for 8 leaves
```

### Clip Nodes

Simplified collision tree:
- Similar structure to BSP nodes
- Rougher boundaries (faster checks)
- Used for first-pass collision detection
- Values: positive=child index, -1=outside model, -2=inside model

#### Reading Lightmap Data from BSP

Here's how to extract and use lightmap data from a BSP file:

```cpp
struct BSPFile {
    uint8_t* lightmapLump;      // Raw lightmap data from file
    int lightmapLumpSize;       // Total bytes in lump
    Face* faces;                // Array of all faces
};

void ProcessFaceLightmap(BSPFile bsp, Face face) {
    // Each face has a lightmap offset into the lump
    if (face.lightmapOffset == -1) {
        // No lightmap for this face (e.g., sky texture)
        return;
    }
    
    // Calculate lightmap dimensions from face extent
    vec3 extent = face.maxBounds - face.minBounds;
    int width = (int)(extent.x / 16) + 1;
    int height = (int)(extent.y / 16) + 1;
    
    // Point to this face's lightmap data
    uint8_t* faceLightmap = bsp.lightmapLump + face.lightmapOffset;
    
    // Read lightmap pixels
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = y * width + x;
            uint8_t brightness = faceLightmap[index];  // 0=dark, 255=bright
            
            // Use this brightness value for rendering
            // Modern engines upload this to a texture atlas
        }
    }
}
```

### Leaves

Terminal BSP nodes containing:
- **type**: Content type (solid, water, lava, slime, sky)
- **vislist**: Index to PVS data
- **faces**: Indices to face list
- **bounds**: Bounding box
- **ambient sound levels**: Water, sky, slime, lava

Leaf types affect rendering and gameplay:
- **-1**: Normal
- **-2**: Solid (not rendered)
- **-3**: Water (blurred vision)
- **-4**: Slime (damages player)
- **-5**: Lava (heavy damage, red tint)
- **-6**: Sky behavior

## File Size Considerations

BSP files are significantly larger than MAP files due to:
- Embedded textures with mipmaps (+33% texture size)
- Duplicate vertices (no vertex sharing across faces)
- Pre-calculated visibility data
- Lightmap data

Typical BSP sizes range from **1-20 MB** depending on:
- Number of textures
- Geometric complexity
- Lightmap resolution
- Visibility complexity

## Processing Pipeline

MAP → BSP compilation involves:

1. **Brush CSG**: Clip intersecting brushes
2. **BSP Tree Generation**: Recursively partition space
3. **Face Generation**: Create convex polygons from brushes
4. **Portal Generation**: Calculate leaf-to-leaf portals
5. **Visibility Calculation**: Ray-trace PVS data
6. **Lightmap Generation**: Radiosity lighting calculations
7. **Data Packing**: Organize into optimized lumps

This process can take from seconds to hours depending on:
- Level complexity
- Visibility calculation settings
- Lighting quality settings

## Limitations

- **Max Vertices**: ~65535 (16-bit indices)
- **Max Faces**: Implementation dependent
- **Max Texture Size**: Must be multiple of 8
- **Face Extent**: Max 256 units (for mipmap selection)
- **Coordinate Range**: ~±32700 (for 16-bit bounding boxes)
- **Static Only**: No animated geometry (only texture animation)

## Tools

Common BSP compilation tools:
- **QBSP**: Generates BSP tree and basic geometry
- **LIGHT**: Calculates lightmaps
- **VIS**: Generates visibility data

Modern tools often combine these into a single step.

## References

- [Quake Specifications - BSP File Format (Section 4)](https://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_4.htm)
- [Quake Specifications - Map Format](https://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_2.htm)
- Michael Abrash's Graphics Programming Black Book (BSP trees)
