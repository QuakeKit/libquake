#include "scene.h"
#include "builder/map_builder.h"
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
      m.model.materials = nullptr;
      UnloadModel(m.model);
    }
    UnloadTexture(currentMapData.lightmapAtlas);
  }

  assetMgr = std::make_unique<AssetManager>(opts.wadPath, opts.texturePath);

  MapBuilder builder(*assetMgr, opts);
  currentMapData = builder.Build(fileName);

  Material *materials =
      assetMgr->BuildMaterialPool(currentMapData.textureNames, renderer->GetMapShader(),
                                  renderer->GetSkyShader(), currentMapData.lightmapAtlas, opts);

  for (auto &qm : currentMapData.models) {
    qm.model.materials = materials;
    qm.model.materialCount = currentMapData.textureNames.size();
  }

  if (currentMapData.playerStart != nullptr) {
    camera.position.x = -currentMapData.playerStart->Origin().x() / opts.inverseScale;
    camera.position.y = currentMapData.playerStart->Origin().z() / opts.inverseScale;
    camera.position.z = currentMapData.playerStart->Origin().y() / opts.inverseScale;
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
      EnableCursor();
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
      m.model.materials = nullptr;
      UnloadModel(m.model);
    }
    UnloadTexture(currentMapData.lightmapAtlas);
  }
}
