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

# Welcome to LibQuake

LibQuake is a C++ library designed to parse, manipulate, and render formats used by the Quake engine (and its derivatives).

## Features

*   **BSP Loading**: Parsing of BSP29 (Quake 1) and related formats.
*   **MAP Parsing**: Reading `.map` source files (Brush geometry).
*   **WAD Handling**: Reading and writing texture archives.
*   **Math Library**: Specialized vector and matrix math for the engine's coordinate system.

## Getting Started

Please see the [Concepts](common/concepts.md) section to understand the coordinate systems and units used.

## Installation

Currently, LibQuake is distributed as a source library. You can include it in your project via CMake.

### CMake Integration

```cmake
add_subdirectory(libs/libquake)
target_link_libraries(your_project PRIVATE libquake)
```

## Quick Start Example

This minimal example demonstrates loading a map, processing it, and finding key game entities.

```cpp
#include <iostream>
#include <quakelib/map/map.h>
#include <quakelib/map/lightmap_generator.h>

int main(int argc, char* argv[]) {
    // 1. Initialize the Map Loader
    quakelib::map::QMap map;

    // Load geometry (throws std::runtime_error on failure)
    try {
        map.LoadFile("maps/e1m1.map");
        std::cout << "Map loaded successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error loading map: " << e.what() << std::endl;
        return 1;
    }

    // 2. Generate Geometry
    // This performs CSG intersection, vertex welding, and polygon triangulation.
    map.GenerateGeometry(true);

    // 3. Generate Lightmaps
    // Create a generator (512x512 atlas, 16 units per luxel)
    quakelib::map::LightmapGenerator lmGen(512, 512, 16.0f);
    
    // Convert map entities pointers to generic types for the generator
    // Note: LightmapGenerator expects std::vector<SolidEntityPtr>
    if (lmGen.Pack(map.SolidEntities())) {
        std::cout << "Lightmap packing successful!" << std::endl;
    } else {
        std::cerr << "Lightmap atlas too small!" << std::endl;
    }

    // 4. Find Player Start
    // Point entities (like info_player_start) are separate from brush entities.
    auto players = map.PointEntitiesByClass("info_player_start");
    
    if (!players.empty()) {
        const auto& start = players[0];
        // Entities have an 'Origin()' helper for the "origin" key
        auto origin = start->Origin();
        
        std::cout << "Found Player Start at: " 
                  << origin[0] << ", " 
                  << origin[1] << ", " 
                  << origin[2] << std::endl;
    }

    // 5. Find All Lights
    // We can iterate through all point entities and filter manually if needed,
    // or use the helper again.
    auto lights = map.PointEntitiesByClass("light");
    std::cout << "Found " << lights.size() << " lights:" << std::endl;

    for (const auto& light : lights) {
        auto origin = light->Origin();
        // Custom attributes can be accessed via String()
        std::string color = light->String("light"); 
        
        std::cout << " - Light at [" 
                  << origin[0] << ", " << origin[1] << ", " << origin[2] 
                  << "] Value: " << (color.empty() ? "Default" : color) << std::endl;
    }

    return 0;
}
```

This example covers the core loop of most Quake-tool applications: Load -> Process -> Iterate Entities.

### Handling Textures & WADs

To generate correct Lightmap UVs, the map system needs to know the dimensions of the textures used. This information resides in WAD files.

```cpp
#include <iostream>
#include <quakelib/map/map.h>
#include <quakelib/wad/wad_manager.h>

int main(int argc, char* argv[]) {
    quakelib::map::QMap map;
    quakelib::wad::QuakeWadManager wadMgr;

    // 1. Load WAD files
    // The texture data is needed for dimension lookup.
    wadMgr.AddWadFile("gfx.wad");

    // 2. Define the callback
    // This lambda will be called by the map loader whenever it needs dimensions
    auto getTextureBounds = [&](const char* textureName) -> quakelib::map::textureBounds {
        // Try to find the texture in our loaded WADs
        auto* tex = wadMgr.FindTexture(textureName);
        if (tex) {
            return { (float)tex->width, (float)tex->height };
        }
        // Fallback for unknown textures (prevents division by zero in UV calc)
        return { 128.0f, 128.0f };
    };

    // 3. Load Map with Callback
    // Passing the callback ensures UVs are calculated correctly during load
    map.LoadFile("maps/e1m1.map", getTextureBounds);

    // 4. Generate Geometry
    map.GenerateGeometry();

    // Now UV coordinates on the map surfaces match the actual texture size.
    std::cout << "Map loaded with correct UV scaling." << std::endl;

    return 0;
}
```
