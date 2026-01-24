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

**LibQuake** is a modern C++ library designed to parse, manipulate, and generate formats used by the Quake engine and its derivatives. It is written in C++20 and intended for tools, map compilers, and engine experiments.

## Features

*   **Map Format Support**: Full parsing of `.map` files (Standard and Valve format).
*   **WAD Management**: Parsing of `.wad` (Texture archives) files.
*   **Geometry Generation**:
    *   Brush construction from planes.
    *   CSG (Constructive Solid Geometry) operations.
    *   Polygon generation and winding.
    *   Texture coordinate calculation (Standard and Valve/Source UVs).
*   **Entity Parsing**: Robust parsing of entity definitions and key-values.

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

