#pragma once

#include "../model/quake_render_model.h"
#include <raylib.h>

class MapRenderer {
public:
  MapRenderer();
  ~MapRenderer();

  void Init();
  void Unload();

  void Draw(const GeneratedMapData &mapData, const Camera &camera, const QuakeMapOptions &opts);

  Shader GetMapShader() const { return mapShader; }

  Shader GetSkyShader() const { return skyShader; }

private:
  Shader mapShader;
  Shader skyShader;

  int wireframeLoc = -1;
  int viewPosLoc = -1;
  int skyTimeLoc = -1;
  int skyViewPosLoc = -1;
};
