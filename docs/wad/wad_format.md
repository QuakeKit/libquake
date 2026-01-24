# WAD (Where's All the Data)

WAD2 files act as containers for textures and other graphical resources in Quake. The format is very similar to DOOM's WAD format but with a slightly different directory entry structure.

## File Structure

### Header
The file starts with a fixed-length header:

```c
typedef struct {
  char magic[4];      // "WAD2"
  long numentries;    // Number of entries in directory
  long diroffset;     // Offset to the directory
} wadhead_t;
```

### Directory
The directory is located at `diroffset` and contains `numentries` of `wadentry_t` structures. Each entry describes a single lump in the WAD file.

```c
typedef struct {
  long offset;        // Position of the entry in WAD file
  long dsize;         // Size of the entry on disk
  long size;          // Size of the entry in memory (uncompressed)
  char type;          // Type of entry
  char cmprs;         // Compression flag (0 = none)
  short dummy;        // Unused padding
  char name[16];      // Name of the entry, null-padded
} wadentry_t;
```

### Entry Types
The `type` field in the directory entry identifies the content of the lump:

| Hex | Char | Description |
|-----|------|-------------|
| 0x40 | '@' | Color Palette |
| 0x42 | 'B' | Status Bar Pictures |
| 0x44 | 'D' | Mipmap Texture (Walls) |
| 0x45 | 'E' | Console Picture |

## Lump Formats

### Miptex (0x44)
Standard wall textures in Quake are stored as `miptex`.
- **Name**: 16-char max string.
- **Dimensions**: Width/Height (must be multiples of 16).
- **Mips**: 4 levels of detail (full, 1/2, 1/4, 1/8 size).
- **Palette**: Pixels are 8-bit indices into the global `QUAKE.PAL`.

### Pictures (0x42, 0x45)
Status bar pictures (0x42) have a simple header followed by raw data:
```c
typedef struct {
  long width;
  long height;
  byte pixels[height][width];
} pichead_t;
```
Console pictures (0x45) are raw dumps without headers, typically 320x200 for backgrounds or 128x128 for character sets.

### Palette (0x40)
The palette lump is a standard 256-color array, identical to DOOM's `PLAYPAL` format:
```c
struct {
    byte r, g, b;
} palette[256];
```

## Reference
- [Quake Spec 3.4 - WAD2 Files](https://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_7.htm)
- [QuakeWiki - WAD2](https://quakewiki.org/wiki/WAD2)
- [QuakeWiki - Palette](https://quakewiki.org/wiki/Palette)
