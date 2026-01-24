#pragma once

#include "../model/quake_render_model.h" // For Options if needed, or colors
#include <map>
#include <quakelib/wad/wad_manager.h>
#include <raylib.h>
#include <string>
#include <vector>

class AssetManager {
public:
  AssetManager(const std::string &wadPath, const std::string &texPath);
  ~AssetManager();

  void LoadWads(const std::vector<std::string> &wadFiles);

  // Returns a pointer to the generated material pool array.
  // The caller (Renderer) takes ownership of the Materials inside (or we manage them here).
  // Actually, Raylib Models store a pointer to materials.
  // Ideally validating lifetime is easier if this class owns them.
  Material *BuildMaterialPool(const std::vector<std::string> &textureNames, Shader mapShader,
                              Shader skyShader, Texture2D lightmapAtlas, const QuakeMapOptions &opts);

  quakelib::wad::QuakeWadManager &GetWadManager() { return wadMgr; }

  void Cleanup();

private:
  std::string wadPath;
  std::string texturePath;
  quakelib::wad::QuakeWadManager wadMgr;

  Material *materialPool = nullptr;
  int materialCount = 0;
};
