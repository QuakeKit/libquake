```text
██╗     ██╗██████╗  ██████╗ ██╗   ██╗ █████╗ ██╗  ██╗███████╗
██║     ██║██╔══██╗██╔═══██╗██║   ██║██╔══██╗██║ ██╔╝██╔════╝
██║     ██║██████╔╝██║   ██║██║   ██║███████║█████╔╝ █████╗
██║     ██║██╔══██╗██║   ██║██║   ██║██╔══██║██╔═██╗ ██╔══╝
███████╗██║██████╔╝╚██████╔╝╚██████╔╝██║  ██║██║  ██╗███████╗
▒▓▓▓▓▓▓░▒▓░▒▓▓▓▓▓▒░ ░▓▓▓▓▓▓░ ░▓▓▓▓▓▓░▒▓░  ▒▓░▒▓░  ▒▓░▒▓▓▓▓▓▓░
░▒▒▒▒▒▒ ░▒ ░▒▒▒▒▒░   ░▒▒▒▒▒   ░▒▒▒▒▒ ░▒   ░▒ ░▒   ░▒ ░▒▒▒▒▒▒
  ░░░░░  ░  ░░░░░      ░░░░     ░░░░  ░    ░  ░    ░  ░░░░░░
    ░        ░          ░        ░                     ░ ░
```

# LibQuake

**LibQuake** is a modern C++ library designed to parse formats used by the Quake engine and its derivatives. It is written in C++20 and intended for tools and engine.

## Supported formats

*   **Map**: Full parsing of `.map` files (Standard and Valve format).
*   **WAD**: Parsing of `.wad` (Texture archives) files.


## Features

*   **Geometry Generation (Map)**:
    *   Brush construction from planes.
    *   CSG (Constructive Solid Geometry) operations.
    *   Polygon generation and winding.
    *   Texture coordinate calculation (Standard and Valve/Source UVs).
*   **Lightmap Generation**:
*   **Entity Parsing**: Parsing of entity definitions and key-values.


## Getting Started

### Prerequisites

*   CMake 3.10+
*   C++20 compliant compiler
*   (Optional) Doxygen for documentation

### Building

```bash
mkdir build
cd build
cmake ..
make
```

### Running Tests

LibQuake uses `snitch` for unit testing.

```bash
cd build
make quakelib_unit_tests
./bin/tests/quakelib_unit_tests
```

## Tools

### qviewer

`qviewer` is a sample application included in `tools/` that demonstrates how to load a map, load associated WADs, and render the geometry.

```bash
./bin/qviewer -w /path/to/wad_dir/ -m /path/to/map.map
```

