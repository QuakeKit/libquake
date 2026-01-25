#include <quakelib/wad/wad.h>
#include <snitch/snitch.hpp>

using namespace quakelib::wad;

TEST_CASE("WAD - Load file", "[wad]") {
  const char *wadPath = "tests/data/prototype.wad";

  SECTION("Load WAD file") {
    QuakeWadOptions opts;
    opts.flipTexHorizontal = false;

    auto wad = QuakeWad::FromFile(wadPath, opts);
    REQUIRE(wad != nullptr);

    const auto &textures = wad->Textures();
    CHECK(textures.size() > 0);

    CAPTURE(textures.size());

    // Note: Textures are loaded lazily - width/height are 0 until GetTexture() is called
    // So we need to actually get textures to validate them

    size_t validTexCount = 0;
    size_t skyTexCount = 0;
    size_t testedCount = 0;

    for (const auto &[name, entry] : textures) {
      // Test a sample of textures (not all 273)
      if (testedCount >= 10)
        break;
      testedCount++;

      QuakeTexture *tex = wad->GetTexture(name);
      if (tex != nullptr) {
        CAPTURE(name);
        CAPTURE(tex->width);
        CAPTURE(tex->height);
        CAPTURE(tex->type);
        CAPTURE(tex->raw.size());

        if (tex->width > 0 && tex->height > 0) {
          validTexCount++;

          // Check that raw data size matches dimensions
          // raw is std::vector<color> where each color is 4 bytes (RGBA)
          size_t expectedPixels = tex->width * tex->height;
          CHECK(tex->raw.size() == expectedPixels);

          if (tex->type == TTYPE_SKY_TEXTURE) {
            skyTexCount++;
          }
        }
      }
    }

    CAPTURE(validTexCount);
    CAPTURE(skyTexCount);
    CAPTURE(testedCount);

    // Should have at least some valid textures
    CHECK(validTexCount > 0);
  }

  SECTION("Get specific texture by name") {
    auto wad = QuakeWad::FromFile(wadPath);
    REQUIRE(wad != nullptr);

    // Try to get a texture that should exist based on the map file
    QuakeTexture *tex = wad->GetTexture("128_blue_3");

    if (tex != nullptr) {
      CHECK(tex->width > 0);
      CHECK(tex->height > 0);

      size_t expectedPixels = tex->width * tex->height;
      CHECK(tex->raw.size() == expectedPixels);

      CAPTURE(tex->width);
      CAPTURE(tex->height);
      CAPTURE(tex->type);
    } else {
      // If not found, list what textures ARE available
      const auto &textures = wad->Textures();
      for (const auto &[name, entry] : textures) {
        CAPTURE(name);
      }
      FAIL("Texture '128_blue_3' not found in WAD");
    }
  }

  SECTION("Get non-existent texture") {
    auto wad = QuakeWad::FromFile(wadPath);
    REQUIRE(wad != nullptr);

    QuakeTexture *tex = wad->GetTexture("DOESNOTEXIST");
    CHECK(tex == nullptr);
  }

  SECTION("Load with horizontal flip") {
    QuakeWadOptions opts;
    opts.flipTexHorizontal = true;

    auto wad = QuakeWad::FromFile(wadPath, opts);
    REQUIRE(wad != nullptr);

    CHECK(wad->Textures().size() > 0);
  }
}

TEST_CASE("WAD - Sky texture detection", "[wad]") {
  CHECK(QuakeWad::IsSkyTexture("sky1") == true);
  CHECK(QuakeWad::IsSkyTexture("sky2") == true);
  CHECK(QuakeWad::IsSkyTexture("skybox") == true);
  CHECK(QuakeWad::IsSkyTexture("METAL1") == false);
  CHECK(QuakeWad::IsSkyTexture("128_blue_3") == false);
}

TEST_CASE("WAD - Invalid file", "[wad]") {
  auto wad = QuakeWad::FromFile("nonexistent.wad");
  CHECK(wad == nullptr);
}
