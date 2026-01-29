#pragma once

#include "../assets/asset_manager.h"
#include "../model/quake_render_model.h"
#include <quakelib/map_provider.h>
#include <string>
#include <vector>

class MapBuilder {
public:
  MapBuilder(AssetManager &assetMgr, const QuakeMapOptions &opts);
  GeneratedMapData Build(quakelib::IMapProviderPtr provider);

private:
  AssetManager &assetMgr;
  QuakeMapOptions opts;

  QuakeModel readModelEntity(quakelib::IMapProviderPtr provider, const quakelib::SolidEntityPtr &ent);
  Texture2D generateLightmapAtlas(quakelib::IMapProviderPtr provider);
};
