#include "asset_manager.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <quakelib/map_provider.h>
#include <quakelib/wad/wad.h>
#include <rlgl.h>

static std::string to_lower(std::string s) {
  for (char &c : s)
    c = tolower(c);
  return s;
}

AssetManager::AssetManager(const std::string &wadPath, const std::string &texPath)
    : wadPath(wadPath), texturePath(texPath) {}

AssetManager::~AssetManager() { Cleanup(); }

void AssetManager::Cleanup() {
  if (materialPool) {
    for (int i = 0; i < materialCount; i++) {
      // we need to prevend shader dealloc, since we share resources.
      materialPool[i].shader.id = rlGetShaderIdDefault();
      UnloadMaterial(materialPool[i]);
    }
    MemFree(materialPool);
    materialPool = nullptr;
    materialCount = 0;
  }
}

void AssetManager::LoadWads(const std::vector<std::string> &wadFiles) {
  for (const auto &wf : wadFiles) {
    std::cout << "Loading WAD: " << (wadPath + wf) << std::endl;
    wadMgr.AddWadFile(wadPath + wf);
  }
}

Material *AssetManager::BuildMaterialPool(const std::vector<std::string> &textureNames, Shader mapShader,
                                          Shader skyShader, Texture2D lightmapAtlas,
                                          const QuakeMapOptions &opts, quakelib::IMapProviderPtr provider) {
  if (materialPool)
    Cleanup();

  materialCount = textureNames.size();
  materialPool = (Material *)MemAlloc(materialCount * sizeof(Material));

  for (size_t i = 0; i < textureNames.size(); ++i) {
    std::string texName = to_lower(textureNames[i]);

    Texture2D rayTex;
    bool found = false;

    auto qt = wadMgr.FindTexture(texName);
    if (qt) {
      Image image;
      size_t size = qt->raw.size() * sizeof(quakelib::wad::color);

      if (!qt->raw.empty()) {
        image.data = RL_MALLOC(size);
        memcpy(image.data, &qt->raw[0], size);
        image.width = qt->width;
        image.height = qt->height;
        image.mipmaps = 1;
        image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        rayTex = LoadTextureFromImage(image);
        found = true;
        RL_FREE(image.data);
      }
    } else if (provider) {
      // Try getting from provider
      auto td = provider->GetTextureData(texName);
      if (td) {
        Image image;
        size_t size = td->data.size();
        image.data = RL_MALLOC(size);
        memcpy(image.data, td->data.data(), size);
        image.width = td->width;
        image.height = td->height;
        image.mipmaps = 1;
        image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        rayTex = LoadTextureFromImage(image);
        found = true;
        RL_FREE(image.data);
      }
    }

    if (!found) {
      // Try loading from disk
      rayTex = LoadTexture((texturePath + texName + ".png").c_str());
    }

    if (rayTex.id <= 0) {
      Material mat = LoadMaterialDefault();
      mat.maps[MATERIAL_MAP_DIFFUSE].color = opts.defaultColor;
      if (mapShader.id > 0)
        mat.shader = mapShader;

      // Assign lightmap even directly (though UVs might be wrong/unused if simple material)
      mat.maps[MATERIAL_MAP_METALNESS].texture = lightmapAtlas;

      materialPool[i] = mat;
    } else {
      GenTextureMipmaps(&rayTex);
      SetTextureFilter(rayTex, TEXTURE_FILTER_ANISOTROPIC_16X);
      Material mat = LoadMaterialDefault();
      mat.maps[MATERIAL_MAP_DIFFUSE].texture = rayTex;
      mat.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;

      bool isSky = quakelib::wad::QuakeWad::IsSkyTexture(texName);

      if (isSky && skyShader.id > 0) {
        mat.shader = skyShader;
      } else {
        mat.maps[MATERIAL_MAP_METALNESS].texture = lightmapAtlas;
        if (mapShader.id > 0)
          mat.shader = mapShader;
      }
      materialPool[i] = mat;
    }
  }
  return materialPool;
}
