# Changelog

All notable changes to this project will be documented in this file.
For historical releases, see [HISTORY.md](HISTORY.md).

---

## v1.0.3 - Lightmap Generation & Non-Solid Brush Export (2026-01-26)

### Added

- **Lightmap Generation API**: Complete lightmap atlas generation system with three new wrapper functions:
  - `QLibMap_GenerateLightmaps()` - Generates lightmap atlas and updates vertex UVs to normalized (0-1) atlas coordinates
  - `QLibMap_CalculateLighting()` - Bakes diffuse lighting from point lights with distance attenuation
  - `QLibMap_GenerateLightmapsAuto()` - Convenience function that automatically extracts light entities from MAP file and bakes lighting
  - `QLibMap_GetLightmapData()` - Exports RGBA lightmap atlas texture
  - `QLibMap_FreeLightmapData()` - Cleanup function
- **QLibMapLight Structure**: Point light definition with position, radius, and RGB color for lightmap baking
- **QLibMapLightmapData Structure**: Lightmap atlas data with width, height, dataSize, and RGBA pixel data
- **Comprehensive Lightmap Documentation**: Complete API reference with usage examples, shader integration guide, and light extraction patterns

### Fixed

- **CLIP/SKIP/NODRAW Brush Export**: Non-solid brushes (CLIP, SKIP, NODRAW, trigger) now properly export without CSG operations. Previously these brushes were being filtered out during CSG, resulting in no mesh output. The CSG system now skips non-solid brushes entirely, allowing them to export with their original geometry.

### Changed

- **CSG Optimization for Non-Solid Brushes**: Added `IsNonSolidBrush()` method to Brush class with cached flag (`m_isNonSolid`) set during geometry generation. Non-solid brushes skip CSG union operations and don't participate in clipping other brushes.
- **Lightmap UV Workflow**: Vertex `lightmapUV` field now populated with normalized atlas coordinates (0-1 range) after calling `QLibMap_GenerateLightmaps()`. Previously contained world-space coordinates.

### Technical Notes

**Lightmap System Architecture**:
- Face packing uses bin-packing algorithm to fit face lightmap regions into atlas
- Luxel size determines lightmap resolution (typical: 16.0 world units per texel)
- Lighting calculation uses simple diffuse model: `attenuation = (1 - distance/radius)²`
- No shadow casting (future enhancement)
- Atlas format is RGBA (4 bytes per pixel)
- Without lighting calculation, returns debug checkerboard pattern

**Non-Solid Brush Handling**:
- All faces in a brush must have the same type (solid or non-solid, never mixed)
- Non-solid status cached during `calculateAABB()` by checking first face type
- CLIP brushes export for collision-only geometry
- SKIP brushes can be used for optimization hints
- NODRAW brushes useful for sealing maps without visible geometry

---

*Previous versions: [v1.0.2](HISTORY.md#v102---enhanced-api--uv-fixes-2026-01-26) | [v1.0.1](HISTORY.md#v101---bug-fixes-2026-01-20) | [v1.0.0](HISTORY.md#v100---initial-release-2026-01-15)*
