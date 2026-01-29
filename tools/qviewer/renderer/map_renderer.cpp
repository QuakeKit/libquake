#include "map_renderer.h"
#include "shaders.h"
#include <raymath.h>
#include <rlgl.h>

MapRenderer::MapRenderer() {
  mapShader.id = 0;
  skyShader.id = 0;
}

MapRenderer::~MapRenderer() { Unload(); }

void MapRenderer::Unload() {
  if (mapShader.id > 0)
    UnloadShader(mapShader);
  if (skyShader.id > 0)
    UnloadShader(skyShader);
  mapShader.id = 0;
  skyShader.id = 0;
}

void MapRenderer::Init() {
  mapShader = LoadShaderFromMemory(VS_CODE, FS_CODE);
  wireframeLoc = GetShaderLocation(mapShader, "renderWireframe");
  viewPosLoc = GetShaderLocation(mapShader, "viewPos");

  skyShader = LoadShaderFromMemory(VS_CODE, FS_SKY_CODE);
  skyTimeLoc = GetShaderLocation(skyShader, "time");
  skyViewPosLoc = GetShaderLocation(skyShader, "viewPos");
}

void MapRenderer::Draw(const GeneratedMapData &mapData, const Camera &camera, const QuakeMapOptions &opts) {
  Vector3 position = {0.0f, 0.0f, 0.0f}; // Models are already in world space, so simplified position 0

  Vector3 cameraPos = camera.position;

  // Update Uniforms
  if (mapShader.id > 0 && viewPosLoc != -1) {
    float camPos[3] = {cameraPos.x, cameraPos.y, cameraPos.z};
    SetShaderValue(mapShader, viewPosLoc, camPos, SHADER_UNIFORM_VEC3);
  }

  if (skyShader.id > 0) {
    if (skyTimeLoc != -1) {
      float time = (float)GetTime();
      SetShaderValue(skyShader, skyTimeLoc, &time, SHADER_UNIFORM_FLOAT);
    }
    if (skyViewPosLoc != -1) {
      float camPos[3] = {cameraPos.x, cameraPos.y, cameraPos.z};
      SetShaderValue(skyShader, skyViewPosLoc, camPos, SHADER_UNIFORM_VEC3);
    }
  }

  // Draw Models
  for (const auto &m : mapData.models) {
    // Ensure materials are set? The Scene/App should have linked them.
    // If materials are nullptr, Raylib might crash or draw pink.

    if (!opts.wireframe) {
      float wireframeVal = 0.0f;
      if (wireframeLoc != -1)
        SetShaderValue(mapShader, wireframeLoc, &wireframeVal, SHADER_UNIFORM_FLOAT);
      DrawModel(m.model, position, 1, WHITE);
    } else {
      float wireframeVal = 1.0f;
      if (wireframeLoc != -1)
        SetShaderValue(mapShader, wireframeLoc, &wireframeVal, SHADER_UNIFORM_FLOAT);

      rlDisableDepthTest();
      rlDisableBackfaceCulling();

      DrawModel(m.model, position, 1, WHITE);

      rlEnableBackfaceCulling();
      rlEnableDepthTest();
    }

    if (opts.showVerts) {
      rlEnableShader(0);
      rlBegin(RL_LINES);
      rlColor4ub(0, 0, 255, 255);
      float k = 0.5f;
      for (int i = 0; i < m.model.meshCount; i++) {
        auto mesh = m.model.meshes[i];
        for (int j = 0; j < mesh.vertexCount; j++) {
          float x = mesh.vertices[j * 3];
          float y = mesh.vertices[j * 3 + 1];
          float z = mesh.vertices[j * 3 + 2];

          rlVertex3f(x - k, y, z);
          rlVertex3f(x + k, y, z);
          rlVertex3f(x, y - k, z);
          rlVertex3f(x, y + k, z);
          rlVertex3f(x, y, z - k);
          rlVertex3f(x, y, z + k);
        }
      }
      rlEnd();
    }
  }
}
