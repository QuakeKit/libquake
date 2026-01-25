# Map System

The Map system provides functionality for loading, parsing, and processing Quake `.map` source files. These are the editable text-based level formats used by level editors.

## Overview

The Map system handles:
- Parsing `.map` file syntax (Standard and Valve 220 formats)
- Building brush geometry from plane definitions
- Performing CSG (Constructive Solid Geometry) operations
- Generating triangulated meshes
- Texture coordinate calculation
- Entity management

## Usage

```cpp
#include <quakelib/map/map.h>

// Create and configure
quakelib::map::QMapConfig config;
config.csg = true;                  // Enable CSG operations
config.convertCoordToOGL = false;   // Coordinate conversion

quakelib::map::QMap map(config);

// Load from file
map.LoadFile("maps/mymap.map");

// Or load from string buffer
map.LoadBuffer(mapData);

// Generate geometry
map.GenerateGeometry();

// Access entities
auto solidEntities = map.SolidEntities();
auto pointEntities = map.PointEntities();
```

## Configuration

The `QMapConfig` structure provides control over geometry processing:

- **`csg`** (default: `true`): Enable CSG operations to clip intersecting brushes
- **`convertCoordToOGL`** (default: `false`): Convert from Quake to OpenGL coordinate system

CSG operations perform brush-to-brush clipping to create proper intersections and prevent overlapping geometry. Disabling CSG will render brushes without clipping, which may result in visual artifacts but is faster for preview purposes.

## Classes

### QMap Class
::: doxy.libquake.class
  name: quakelib::map::QMap

### QMapConfig Structure
::: doxy.libquake.struct
  name: quakelib::map::QMapConfig

### Brush Class
::: doxy.libquake.class
  name: quakelib::map::Brush

### Face Class
::: doxy.libquake.class
  name: quakelib::map::MapSurface

### Solid Entity Class
::: doxy.libquake.class
  name: quakelib::map::SolidMapEntity
