#include "scene.h"
#include "builder/map_builder.h"
#include "common_utils.h"
#include "metrics.h"
#include <filesystem>
#include <quakelib/bsp/qbsp_provider.h>
#include <quakelib/map/qmap_provider.h>
#include <raymath.h>
#include <string>

Scene::Scene(int width, int height) {
  SetTraceLogLevel(LOG_ERROR);
  SetConfigFlags(FLAG_MSAA_4X_HINT);
  InitWindow(width, height, "Quake Map Viewer");
  SetTargetFPS(120);
  camera = {{2.0f, 5.0f, 2.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 45.0f, 0};

  renderer = std::make_unique<MapRenderer>();
  renderer->Init();
}

void Scene::LoadQuakeMap(const std::string &fileName, QuakeMapOptions opts) {
  currentOpts = opts;

  if (isMapLoaded) {
    for (auto &m : currentMapData.models) {
      UnloadModel(m.model);
      m.model.materials = nullptr;
    }
    UnloadTexture(currentMapData.lightmapAtlas);
  }

  assetMgr = std::make_unique<AssetManager>(opts.wadPath, opts.texturePath);

  std::filesystem::path path(fileName);
  std::shared_ptr<quakelib::IMapProvider> provider;

  if (path.extension() == ".bsp" || path.extension() == ".BSP") {
    provider = std::make_shared<quakelib::QBspProvider>();
  } else {
    provider = std::make_shared<quakelib::QMapProvider>();
  }

  if (!provider->Load(fileName)) {
    TraceLog(LOG_ERROR, "Failed to load map/bsp file");
    return;
  }

  // Load WADs specified in map (if any)
  auto wads = provider->GetRequiredWads();
  if (!wads.empty()) {
    assetMgr->LoadWads(wads);
  }

  // Register texture bounds provider using AssetManager
  provider->SetTextureBoundsProvider([this](const std::string &name) -> std::pair<int, int> {
    std::string sName = to_lower(name);
    auto qt = this->assetMgr->GetWadManager().FindTexture(sName);
    if (qt) {
      return {qt->width, qt->height};
    }
    return {0, 0};
  });

  // Configure default types for both
  provider->SetFaceType("clip", quakelib::SurfaceType::CLIP);
  provider->SetFaceType("trigger", quakelib::SurfaceType::CLIP);
  provider->SetFaceType("skip", quakelib::SurfaceType::SKIP);

  Metrics::instance().startTimer("geo_generate");
  provider->GenerateGeometry(true);
  Metrics::instance().finalizeTimer("geo_generate");

  MapBuilder builder(*assetMgr, opts);
  currentMapData = builder.Build(provider);

  Material *materials =
      assetMgr->BuildMaterialPool(currentMapData.textureNames, renderer->GetMapShader(),
                                  renderer->GetSkyShader(), currentMapData.lightmapAtlas, opts, provider);

  for (auto &qm : currentMapData.models) {
    qm.model.materials = materials;
    qm.model.materialCount = currentMapData.textureNames.size();
  }

  if (currentMapData.playerStart != nullptr) {
    camera.position.x = -currentMapData.playerStart->Origin().X / opts.inverseScale;
    camera.position.y = currentMapData.playerStart->Origin().Z / opts.inverseScale;
    camera.position.z = currentMapData.playerStart->Origin().Y / opts.inverseScale;
    auto angle = (currentMapData.playerStart->Angle() - 90) * DEG2RAD;
    auto target = Vector3Transform((Vector3){0, 0, 1}, MatrixRotateZYX((Vector3){0, -angle, 0}));

    camera.target.x = camera.position.x + target.x;
    camera.target.y = camera.position.y + target.y;
    camera.target.z = camera.position.z + target.z;
  }

  isMapLoaded = true;
}

void Scene::Run() {
  auto lpos = (Vector3){-2, 5, -2};
  DisableCursor();
  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_M)) {
      IsCursorHidden() ? EnableCursor() : DisableCursor();
    }

    if (IsKeyPressed(KEY_V))
      currentOpts.showVerts = !currentOpts.showVerts;
    if (IsKeyPressed(KEY_L))
      currentOpts.wireframe = !currentOpts.wireframe;
    if (IsKeyPressed(KEY_G))
      currentOpts.showGrid = !currentOpts.showGrid;

    if (IsCursorHidden()) {
      UpdateCamera(&camera, CAMERA_FREE);
    }

    Vector3 cameraPos = camera.position;

    BeginDrawing();
    ClearBackground(currentOpts.backgroundColor);

    BeginMode3D(camera);

    if (isMapLoaded) {
      renderer->Draw(currentMapData, camera, currentOpts);
    }

    if (currentOpts.showGrid) {
      DrawGrid(64, 1.0f);
    }
    EndMode3D();

    DrawFPS(10, 10);
    DrawText("WASD+Mouse: Move | V: Verts | L: Lines | G: Grid | M: Mouse", 10, 30, 20, DARKGRAY);

    EndDrawing();
  }

  if (isMapLoaded) {
    for (auto &m : currentMapData.models) {
      m.model.materialCount = 0;
      for (int i = 0; i < m.model.meshCount; i++)
        UnloadMesh(m.model.meshes[i]);
    }
    UnloadTexture(currentMapData.lightmapAtlas);
  }
}
