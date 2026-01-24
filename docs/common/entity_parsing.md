# Entity Parsing

The Entity Parser is responsible for reading the text-based description of the game world from `.map` files (or similar formats) and converting them into in-memory objects.

## File Format

Quake-based map files use a hierarchical text format. The parser expects a structure of nested curly braces `{ }`.

### Structure
1.  **Entities**: Top-level blocks enclosed in `{ }`.
2.  **Properties**: lines inside an entity formatted as `"key" "value"`.
3.  **Brushes**: Nested `{ }` blocks inside an entity, defining the geometry (planes).

Example:
```quake
// Worldspawn Entity
{
"classname" "worldspawn"
"message" "My Map"
    // Brush 1
    {
    ( 0 0 0 ) ( 0 64 0 ) ( 0 0 64 ) TEXTURE 0 0 0 1.0 1.0
    ...
    }
}

// Point Entity
{
"classname" "info_player_start"
"origin" "0 0 64"
}
```

## Parsing Logic (`EntityParser`)

The `EntityParser` class (`include/quakelib/entity_parser.h`) reads the input stream line-by-line using a simple state machine.

### Key Logic
*   **Scopes**: It tracks depth using a `current` pointer.
    *   Found `{`: push new `ParsedEntity`, set as child of current (if any).
    *   Found `}`: pop to parent.
*   **Content**: Any line that is not `{` or `}` is appended to the `lines` buffer of the current `ParsedEntity`.
*   **Format Detection**: It includes basic detection for formats like Valve 220 (via specific comments).

### Usage

```cpp
#include <quakelib/entity_parser.h>

void OnEntityParsed(quakelib::ParsedEntity* rawEnt) {
    if (rawEnt->type == quakelib::EntityType::WORLDSPAWN) {
        // Handle world geometry
    }
    
    // Process key-values
    // (Usually handled by converting ParsedEntity to a strictly typed Entity)
}

// ...
std::ifstream file("maps/e1m1.map");
quakelib::EntityParser::ParseEntites(file, OnEntityParsed);
```

### Entity Construction
The `ParsedEntity` struct is an intermediate raw representation. It is usually converted into a full `Entity` object via `Entity::FillFromParsed`.
*   **Key-Values**: Extracted using Regex (`\"([\s\S]*?)\"`).
*   **Brushes**: Child entities that do not have key-values are often interpreted as Brushes (Solid geometry).

## Reference
*   **Quake Map Format**: [QuakeWiki - Quake Map Format](https://quakewiki.org/wiki/Quake_Map_Format)
