# Changelog

## v1.0.0 - Initial Release

### Features

- **BSP File Support**: Load and parse Quake BSP files with comprehensive metadata
  - Texture loading with optional texture data
  - Lightmap support with UV coordinates
  - Entity parsing
  - Coordinate system conversion (Quake to OpenGL)

- **MAP File Support**: Load and process Quake MAP source files
  - Standard and Valve 220 format support
  - CSG (Constructive Solid Geometry) operations
  - Texture coordinate calculation
  - Brush clipping and geometry generation

- **WAD File Support**: Load and extract textures from WAD archives
  - Texture metadata (width, height, type)
  - Mipmap support
  - Palette data

- **Unified C-Style Wrapper API**: Easy integration with C# and Unity
  - Batch-export functions for efficient data transfer
  - Contiguous memory layouts
  - QLib-prefixed naming convention

- **Coordinate System Conversion**: Seamless conversion between Quake and OpenGL coordinate systems

- **Documentation**: Comprehensive documentation with code examples
  - API documentation for all classes
  - Format specifications
  - Usage examples
  - Lightmapping algorithms explained with code

### Technical Details

- Cross-platform support (Windows, macOS, Linux)
- Universal binary for macOS (arm64 + x86_64)
- Modern C++20 implementation
- CMake build system
- Includes qviewer tool for visualization
