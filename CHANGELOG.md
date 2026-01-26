# Changelog

All notable changes to this project will be documented in this file.
For historical releases, see [HISTORY.md](HISTORY.md).

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

*Previous versions: [v1.0.1](HISTORY.md#v101---bug-fixes-2026-01-20) | [v1.0.0](HISTORY.md#v100---initial-release-2026-01-15)*
