#include "../../src/wrapper.h"
#include <cstring>
#include <snitch/snitch.hpp>
#include <string>

TEST_CASE("Wrapper API - Load MAP file", "[wrapper][map]") {
  const char *mapPath = "tests/data/test.map";

  SECTION("Load MAP with CSG enabled") {
    void *map = QLibMap_Load(mapPath, 1, 1);
    REQUIRE(map != nullptr);

    SECTION("Export all MAP data") {
      QLibMapData *data = QLibMap_ExportAll(map);
      REQUIRE(data != nullptr);

      // Check basic counts
      CAPTURE(data->solidEntityCount);
      CAPTURE(data->pointEntityCount);
      CAPTURE(data->textureCount);
      CAPTURE(data->requiredWadCount);

      // Should have at least worldspawn
      CHECK(data->solidEntityCount >= 1);

      // Check required WADs
      CHECK(data->requiredWadCount > 0);
      REQUIRE(data->requiredWads != nullptr);

      // Should have prototype.wad
      bool foundPrototype = false;
      for (uint32_t i = 0; i < data->requiredWadCount; i++) {
        if (std::strcmp(data->requiredWads[i], "prototype.wad") == 0) {
          foundPrototype = true;
        }
      }
      CHECK(foundPrototype);

      // Check texture names
      CHECK(data->textureCount > 0);
      REQUIRE(data->textureNames != nullptr);

      // Should have 128_blue_3 texture
      bool foundBlueTexture = false;
      for (uint32_t i = 0; i < data->textureCount; i++) {
        if (std::strcmp(data->textureNames[i], "128_blue_3") == 0) {
          foundBlueTexture = true;
        }
      }
      CHECK(foundBlueTexture);

      // Validate worldspawn
      if (data->solidEntityCount > 0) {
        QLibMapEntityMesh *world = &data->solidEntities[0];

        CHECK(std::strcmp(world->className, "worldspawn") == 0);
        CHECK(world->submeshCount > 0);
        CHECK(world->totalVertexCount > 0);
        CHECK(world->totalIndexCount > 0);

        // Verify vertices are allocated
        REQUIRE(world->vertices != nullptr);
        REQUIRE(world->indices != nullptr);
        REQUIRE(world->submeshes != nullptr);

        // Check first submesh
        QLibMapSubmesh *submesh = &world->submeshes[0];
        CHECK(submesh->vertexCount > 0);
        CHECK(submesh->indexCount > 0);
        CHECK(std::strlen(submesh->textureName) > 0);

        CAPTURE(submesh->vertexCount);
        CAPTURE(submesh->indexCount);
        CAPTURE(submesh->surfaceType);

        // Validate texture name is set
        CHECK(submesh->textureName[0] != '\0');

        // Validate vertex data
        QLibVertex *firstVert = &world->vertices[0];
        CHECK(firstVert != nullptr);

        // Check attributes
        CHECK(world->attributeCount > 0);
        REQUIRE(world->attributeKeys != nullptr);
        REQUIRE(world->attributeValues != nullptr);

        bool foundClassname = false;
        bool foundWad = false;
        for (uint32_t i = 0; i < world->attributeCount; i++) {
          if (std::strcmp(world->attributeKeys[i], "classname") == 0) {
            foundClassname = true;
            CHECK(std::strcmp(world->attributeValues[i], "worldspawn") == 0);
          }
          if (std::strcmp(world->attributeKeys[i], "wad") == 0) {
            foundWad = true;
          }
        }
        CHECK(foundClassname);
        CHECK(foundWad);
      }

      QLibMap_FreeData(data);
    }

    QLibMap_Destroy(map);
  }

  SECTION("Load MAP with CSG disabled") {
    void *map = QLibMap_Load(mapPath, 0, 1);
    REQUIRE(map != nullptr);

    QLibMapData *data = QLibMap_ExportAll(map);
    REQUIRE(data != nullptr);

    // Should still have data even without CSG
    CHECK(data->solidEntityCount >= 1);

    QLibMap_FreeData(data);
    QLibMap_Destroy(map);
  }

  SECTION("Get specific entity mesh") {
    void *map = QLibMap_Load(mapPath, 1, 1);
    REQUIRE(map != nullptr);

    QLibMapEntityMesh *mesh = QLibMap_GetEntityMesh(map, 0);
    REQUIRE(mesh != nullptr);

    CHECK(std::strcmp(mesh->className, "worldspawn") == 0);
    CHECK(mesh->totalVertexCount > 0);
    CHECK(mesh->submeshCount > 0);

    QLibMap_FreeMesh(mesh);
    QLibMap_Destroy(map);
  }

  SECTION("Invalid entity index returns NULL") {
    void *map = QLibMap_Load(mapPath, 1, 1);
    REQUIRE(map != nullptr);

    QLibMapEntityMesh *mesh = QLibMap_GetEntityMesh(map, 9999);
    CHECK(mesh == nullptr);

    QLibMap_Destroy(map);
  }
}

TEST_CASE("Wrapper API - Set face types", "[wrapper][map]") {
  const char *mapPath = "tests/data/test.map";

  void *map = QLibMap_Load(mapPath, 1, 1);
  REQUIRE(map != nullptr);

  // Note: SetFaceType must be called before GenerateGeometry() in the actual workflow
  // Since QLibMap_Load already calls GenerateGeometry(), this test just verifies
  // the function doesn't crash
  QLibMap_SetFaceType(map, "128_blue_3", 1);

  QLibMap_Destroy(map);
}

TEST_CASE("Wrapper API - Load WAD file", "[wrapper][wad]") {
  const char *wadPath = "tests/data/prototype.wad";

  SECTION("Load WAD file") {
    void *wad = QLibWad_Load(wadPath, 0);
    REQUIRE(wad != nullptr);

    QLibWadData *data = QLibWad_ExportAll(wad);
    REQUIRE(data != nullptr);

    CHECK(data->textureCount > 0);
    REQUIRE(data->textures != nullptr);

    CAPTURE(data->textureCount);

    // Find valid textures (width/height will be 0 until GetTexture is called in C++ API)
    // But the wrapper should call GetTexture for us... or we need to document lazy loading
    size_t validCount = 0;
    for (uint32_t i = 0; i < data->textureCount && i < 10; i++) {
      QLibWadTexture *tex = &data->textures[i];
      if (tex->width > 0 && tex->height > 0 && tex->data != nullptr) {
        validCount++;
        // Data should be RGBA: width * height * 4 bytes
        CHECK(tex->dataSize == tex->width * tex->height * 4);
      }
    }

    QLibWad_FreeData(data);
    QLibWad_Destroy(wad);
  }

  SECTION("Get specific texture") {
    void *wad = QLibWad_Load(wadPath, 0);
    REQUIRE(wad != nullptr);

    QLibWadTexture *tex = QLibWad_GetTexture(wad, "128_blue_3");
    REQUIRE(tex != nullptr);

    CHECK(tex->width > 0);
    CHECK(tex->height > 0);
    CHECK(tex->dataSize == tex->width * tex->height * 4); // RGBA
    CHECK(tex->data != nullptr);
    CHECK(std::strcmp(tex->name, "128_blue_3") == 0);

    QLibWad_FreeTexture(tex);
    QLibWad_Destroy(wad);
  }

  SECTION("Get non-existent texture returns NULL") {
    void *wad = QLibWad_Load(wadPath, 0);
    REQUIRE(wad != nullptr);

    QLibWadTexture *tex = QLibWad_GetTexture(wad, "DOESNOTEXIST");
    CHECK(tex == nullptr);

    QLibWad_Destroy(wad);
  }

  SECTION("Load with horizontal flip") {
    void *wad = QLibWad_Load(wadPath, 1);
    REQUIRE(wad != nullptr);

    QLibWadTexture *tex = QLibWad_GetTexture(wad, "128_blue_3");
    if (tex != nullptr) {
      CHECK(tex->width > 0);
      QLibWad_FreeTexture(tex);
    }

    QLibWad_Destroy(wad);
  }
}

TEST_CASE("Wrapper API - NULL safety", "[wrapper]") {
  SECTION("NULL pointers are handled") {
    CHECK(QLibMap_ExportAll(nullptr) == nullptr);
    CHECK(QLibMap_GetEntityMesh(nullptr, 0) == nullptr);
    CHECK(QLibWad_ExportAll(nullptr) == nullptr);
    CHECK(QLibWad_GetTexture(nullptr, "test") == nullptr);

    // These should not crash
    QLibMap_FreeMesh(nullptr);
    QLibMap_FreeData(nullptr);
    QLibMap_Destroy(nullptr);
    QLibWad_FreeTexture(nullptr);
    QLibWad_FreeData(nullptr);
    QLibWad_Destroy(nullptr);
  }

  SECTION("Invalid file paths") {
    // MAP load catches exceptions but behavior on bad path depends on file loading
    void *map = QLibMap_Load("nonexistent.map", 1, 1);
    if (map != nullptr) {
      QLibMap_Destroy(map);
    }

    void *wad = QLibWad_Load("nonexistent.wad", 0);
    CHECK(wad == nullptr);
  }
}
