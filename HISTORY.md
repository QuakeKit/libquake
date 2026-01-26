# Release History

This file contains archived release notes for historical versions.

---

## v1.0.2 - Enhanced API & UV Fixes (2026-01-26)

### Added

- **`QLibMap_GetTextureNames()` API**: New function to retrieve all texture names used in a MAP file, enabling flexible texture loading from any source (disk, memory, WAD, procedural) without being limited to WAD-only workflow.

### Fixed

- **Valve UV Format Parsing**: Fixed mapVersion detection bug that prevented Valve format UV coordinates from being parsed correctly. The parser now properly identifies and handles Valve 220 format MAP files.
- **Bracket Removal in MAP Parser**: Implemented proper bracket removal from MAP file texture definition lines before parsing, matching the behavior of the original libqmap implementation.

### Changed

- **Decoupled Texture Registration**: The texture dimension registration workflow is now fully decoupled from WAD files. Users can call `QLibMap_GetTextureNames()` to get texture requirements, then `QLibMap_RegisterTextureSize()` from any source, before finally calling `QLibMap_GenerateGeometry()`.

### Technical Notes

**UV Calculation Fix**: The root cause of UV coordinates calculating as infinity was that mapVersion wasn't being parsed from MAP files, causing the parser to default to standard format when the file used Valve format. This led to stream misalignment and zero values for scale parameters, resulting in division by zero during UV calculation.

---

## v1.0.1 - Bug Fixes (2026-01-20)

### Fixed

- **WAD Wrapper RGBA Conversion**: Fixed incorrect byte size calculation when converting texture data from internal `std::vector<color>` to C API byte array. Was setting `dataSize` to pixel count instead of `pixel_count * 4`, causing truncated texture data export.
- **Memory Leak in WAD Loading**: Fixed leak in `QuakeWad::GetTexture()` where dynamically allocated texture from `FromBuffer()` was copied but never freed.

### Added

- **C Wrapper API Documentation**: Comprehensive documentation with struct memory layouts and code examples for WAD, BSP, and MAP APIs.
- **Unit Tests**: Added extensive test coverage for WAD loading and wrapper API validation.

### Technical Notes

**Why `textureName` in submesh?** Including the texture name directly in each submesh makes it self-contained for FFI consumers—no need to maintain separate lookup tables. It also handles missing textures gracefully (when `textureID` is -1) and provides better error messages during debugging.

---

## v1.0.0 - Initial Release (2026-01-15)

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
