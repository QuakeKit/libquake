#include <quakelib/map/brush.h>
#include <quakelib/map/face.h>
#include <quakelib/map/types.h>
#include <snitch/snitch.hpp>

using namespace quakelib;
using namespace quakelib::map;

Brush CreateBlock(Vec3 min, Vec3 max) {
  Brush b;

  // Points of the box
  Vec3 p000 = {min[0], min[1], min[2]};
  Vec3 p100 = {max[0], min[1], min[2]};
  Vec3 p110 = {max[0], max[1], min[2]};
  Vec3 p010 = {min[0], max[1], min[2]};

  Vec3 p001 = {min[0], min[1], max[2]};
  Vec3 p101 = {max[0], min[1], max[2]};
  Vec3 p111 = {max[0], max[1], max[2]};
  Vec3 p011 = {min[0], max[1], max[2]};

  // Faces (Derived manually for Outward Normals)

  // Z+ (Top) - Normal (0,0,1) - CW in XY
  b.AddFace(std::make_shared<MapSurface>(std::array<Vec3, 3>{p011, p111, p101}, 0, StandardUV{}, 0, 1, 1));

  // Z- (Bottom) - Normal (0,0,-1) - CCW in XY
  b.AddFace(std::make_shared<MapSurface>(std::array<Vec3, 3>{p000, p100, p110}, 0, StandardUV{}, 0, 1, 1));

  // X+ (Right) - Normal (1,0,0) - CW in YZ
  b.AddFace(std::make_shared<MapSurface>(std::array<Vec3, 3>{p100, p101, p111}, 0, StandardUV{}, 0, 1, 1));

  // X- (Left) - Normal (-1,0,0) - CCW in YZ
  b.AddFace(std::make_shared<MapSurface>(std::array<Vec3, 3>{p000, p010, p011}, 0, StandardUV{}, 0, 1, 1));

  // Y+ (Back) - Normal (0,1,0) - CCW in XZ
  b.AddFace(std::make_shared<MapSurface>(std::array<Vec3, 3>{p110, p111, p011}, 0, StandardUV{}, 0, 1, 1));

  // Y- (Front) - Normal (0,-1,0) - CW in XZ
  b.AddFace(std::make_shared<MapSurface>(std::array<Vec3, 3>{p000, p001, p101}, 0, StandardUV{}, 0, 1, 1));

  return b;
}

TEST_CASE("Brush Geometry Build", "[map/construction]") {
  Brush b = CreateBlock({0, 0, 0}, {64, 64, 64});

  // dummy tex bounds
  std::map<int, textureBounds> texBounds;
  texBounds[0] = {64, 64};
  std::map<int, MapSurface::eFaceType> faceTypes;
  faceTypes[0] = MapSurface::SOLID;

  b.buildGeometry(faceTypes, texBounds);

  // A cube has 6 faces.
  REQUIRE(b.Faces().size() == 6);

  for (auto &f : b.Faces()) {
    // Each face should intersect with others to form a quad (4 verts).
    REQUIRE(f->Vertices().size() == 4);
  }
}

TEST_CASE("Brush Intersection", "[map/csg]") {
  // Setup context for building
  std::map<int, textureBounds> texBounds;
  texBounds[0] = {64, 64};
  std::map<int, MapSurface::eFaceType> faceTypes;
  faceTypes[0] = MapSurface::SOLID;

  // 1. Base Brush
  Brush b1 = CreateBlock({0, 0, 0}, {100, 100, 100});
  b1.buildGeometry(faceTypes, texBounds);

  // 2. Overlapping Brush (Center)
  Brush b2 = CreateBlock({25, 25, 25}, {75, 75, 75});
  b2.buildGeometry(faceTypes, texBounds);

  REQUIRE(b1.DoesIntersect(b2));
  REQUIRE(b2.DoesIntersect(b1));

  // 3. Overlapping Brush (Corner)
  Brush b3 = CreateBlock({80, 80, 80}, {120, 120, 120});
  b3.buildGeometry(faceTypes, texBounds);

  REQUIRE(b1.DoesIntersect(b3));

  // 4. Disjoint Brush
  Brush b4 = CreateBlock({200, 200, 200}, {300, 300, 300});
  b4.buildGeometry(faceTypes, texBounds);

  REQUIRE_FALSE(b1.DoesIntersect(b4));

  // 5. Touching Brush (Share face)
  Brush b5 = CreateBlock({100, 0, 0}, {200, 100, 100});
  b5.buildGeometry(faceTypes, texBounds);

  REQUIRE(b1.DoesIntersect(b5));
}
