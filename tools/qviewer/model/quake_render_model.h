#pragma once

#include <quakelib/map/entities.h>
#include <raylib.h>
#include <string>
#include <vector>

struct QuakeModel {
  Model model;
  std::vector<Mesh> meshes;
  std::vector<int> materialIDs;
};

struct QuakeMapOptions {
  std::string texturePath = "textures/";
  std::string wadPath = "wads/";
  float inverseScale = 24.0f;
  bool showGrid = false;
  bool showVerts = false;
  bool wireframe = false;
  Color backgroundColor = WHITE;
  Color defaultColor = WHITE;
  Color wireframeColor = BLUE;
  float lightmapMultiplier = 1.0f;
};

struct GeneratedMapData {
  std::vector<QuakeModel> models;
  Texture2D lightmapAtlas;
  quakelib::map::PointEntityPtr playerStart;
  std::vector<std::string> textureNames;
};
