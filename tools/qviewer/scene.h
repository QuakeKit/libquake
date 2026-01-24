#pragma once

#include "assets/asset_manager.h"
#include "model/quake_render_model.h"
#include "renderer/map_renderer.h"
#include <memory>
#include <raylib.h>

class Scene {
public:
  Scene(int width = 800, int height = 600);
  void LoadQuakeMap(const std::string &fileName, QuakeMapOptions opts);
  void Run();

private:
  Camera camera;
  GeneratedMapData currentMapData;
  std::unique_ptr<AssetManager> assetMgr;
  std::unique_ptr<MapRenderer> renderer;

  QuakeMapOptions currentOpts;
  bool isMapLoaded = false;
};
