#pragma once

#include "../assets/asset_manager.h"
#include "../model/quake_render_model.h"
#include <quakelib/map/entities.h>
#include <quakelib/map/map.h>
#include <string>

class MapBuilder {
public:
  MapBuilder(AssetManager &assetMgr, const QuakeMapOptions &opts);
  GeneratedMapData Build(const std::string &filename);

private:
  AssetManager &assetMgr;
  QuakeMapOptions opts;
  quakelib::map::QMap mapInstance;

  QuakeModel readModelEntity(const quakelib::map::SolidEntityPtr &ent);
  Texture2D generateLightmapAtlas();
};
