#include "map_builder.h"
#include "../common_utils.h"
#include "../metrics.h"
#include <cstring>
#include <iostream>
#include <quakelib/bsp/qbsp_provider.h>
#include <quakelib/map/lightmap_generator.h>
#include <quakelib/map/qmap_provider.h>
#include <quakelib/wad/wad.h>
#include <raymath.h>
#include <rlgl.h>

MapBuilder::MapBuilder(AssetManager &assetMgr, const QuakeMapOptions &opts)
    : assetMgr(assetMgr), opts(opts) {}

GeneratedMapData MapBuilder::Build(quakelib::IMapProviderPtr provider) {
  GeneratedMapData data;

  data.lightmapAtlas = generateLightmapAtlas(provider);

  // Convert entities to models (includes xatlas UV generation per mesh)
  Metrics::instance().startTimer("mesh_convert");
  for (const auto &se : provider->GetSolidEntities()) {
    auto m = readModelEntity(provider, se);
    // Only add if it has meshes
    if (m.model.meshCount > 0) {
      data.models.push_back(m);

      // Print vertex count for worldspawn
      if (se->ClassName() == "worldspawn") {
        int totalVertices = 0;
        for (int i = 0; i < m.model.meshCount; i++) {
          totalVertices += m.model.meshes[i].vertexCount;
        }
        std::cout << "Worldspawn vertex count: " << totalVertices << std::endl;
      }
    }
  }
  Metrics::instance().finalizeTimer("mesh_convert");

  if (auto pointEnts = provider->GetPointEntities("info_player_start"); pointEnts.size() > 0) {
    data.playerStart = pointEnts[0];
  }

  data.textureNames = provider->GetTextureNames();

  return data;
}

Texture2D MapBuilder::generateLightmapAtlas(quakelib::IMapProviderPtr provider) {
  // Check if provider has existing lightmap (typical for BSP)
  auto existingLm = provider->GetLightmapData();
  if (existingLm) {
    std::cout << "Using existing BSP lightmap" << std::endl;
    Image img;
    img.data = RL_MALLOC(existingLm->data.size());
    memcpy(img.data, existingLm->data.data(), existingLm->data.size());
    img.width = existingLm->width;
    img.height = existingLm->height;
    img.mipmaps = 1;
    img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    return tex;
  }

  // Return white texture placeholder
  std::cout << "Using white lightmap placeholder" << std::endl;
  Image whiteImg = GenImageColor(512, 512, WHITE);
  Texture2D tex = LoadTextureFromImage(whiteImg);
  UnloadImage(whiteImg);
  SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
  return tex;
}

QuakeModel MapBuilder::readModelEntity(quakelib::IMapProviderPtr provider,
                                       const quakelib::SolidEntityPtr &ent) {
  QuakeModel qm = {0};

  auto meshes = provider->GetEntityMeshes(ent);

  for (const auto &mesh : meshes) {
    if (mesh.vertices.empty())
      continue;

    // Check type
    if (mesh.type == quakelib::SurfaceType::CLIP || mesh.type == quakelib::SurfaceType::SKIP ||
        mesh.type == quakelib::SurfaceType::NODRAW)
      continue;

    auto rayMesh = Mesh{0};
    rayMesh.triangleCount = mesh.indices.size() / 3;
    rayMesh.vertexCount = mesh.vertices.size();

    rayMesh.vertices = (float *)MemAlloc(rayMesh.vertexCount * 3 * sizeof(float));
    rayMesh.tangents = (float *)MemAlloc(rayMesh.vertexCount * 4 * sizeof(float));
    rayMesh.texcoords = (float *)MemAlloc(rayMesh.vertexCount * 2 * sizeof(float));
    rayMesh.texcoords2 = (float *)MemAlloc(rayMesh.vertexCount * 2 * sizeof(float));
    rayMesh.normals = (float *)MemAlloc(rayMesh.vertexCount * 3 * sizeof(float));
    rayMesh.colors = (unsigned char *)MemAlloc(rayMesh.vertexCount * 4 * sizeof(unsigned char));

    // We must invoke indices allocation even if we populate manually?
    // Raylib UploadMesh handles indices if provided.
    rayMesh.indices = (unsigned short *)MemAlloc(mesh.indices.size() * sizeof(unsigned short));

    unsigned char baryColors[3][4] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};

    // Copy data
    for (int i = 0; i < mesh.vertices.size(); ++i) {
      // Coordinate conversion: Z-up to Y-up
      rayMesh.vertices[i * 3 + 0] = -mesh.vertices[i].point[0] / opts.inverseScale;
      rayMesh.vertices[i * 3 + 1] = mesh.vertices[i].point[2] / opts.inverseScale;
      rayMesh.vertices[i * 3 + 2] = mesh.vertices[i].point[1] / opts.inverseScale;

      rayMesh.normals[i * 3 + 0] = -mesh.vertices[i].normal[0];
      rayMesh.normals[i * 3 + 1] = mesh.vertices[i].normal[2];
      rayMesh.normals[i * 3 + 2] = mesh.vertices[i].normal[1];

      // Tangents
      rayMesh.tangents[i * 4 + 0] = -mesh.vertices[i].tangent[0];
      rayMesh.tangents[i * 4 + 1] = mesh.vertices[i].tangent[2];
      rayMesh.tangents[i * 4 + 2] = mesh.vertices[i].tangent[1];
      rayMesh.tangents[i * 4 + 3] = 1.0f;

      rayMesh.texcoords[i * 2 + 0] = mesh.vertices[i].uv[0];
      rayMesh.texcoords[i * 2 + 1] = mesh.vertices[i].uv[1];

      rayMesh.texcoords2[i * 2 + 0] = mesh.vertices[i].lightmap_uv[0];
      rayMesh.texcoords2[i * 2 + 1] = mesh.vertices[i].lightmap_uv[1];

      // Barycentric colors for debugging
      int triIndex = i % 3;
      rayMesh.colors[i * 4 + 0] = baryColors[triIndex][0];
      rayMesh.colors[i * 4 + 1] = baryColors[triIndex][1];
      rayMesh.colors[i * 4 + 2] = baryColors[triIndex][2];
      rayMesh.colors[i * 4 + 3] = baryColors[triIndex][3];
    }

    for (int i = 0; i < mesh.indices.size(); ++i) {
      rayMesh.indices[i] = (unsigned short)mesh.indices[i];
    }

    for (int i = 0; i < mesh.indices.size(); i += 3) {
      std::swap(rayMesh.indices[i], rayMesh.indices[i + 2]);
    }

    UploadMesh(&rayMesh, false);
    qm.meshes.push_back(rayMesh);

    auto names = provider->GetTextureNames();
    auto it = std::find(names.begin(), names.end(), mesh.textureName);
    int matID = 0;
    if (it != names.end()) {
      matID = std::distance(names.begin(), it);
    }
    qm.materialIDs.push_back(matID);
  }

  // ... rest of setup ...

  qm.model.transform = MatrixIdentity();
  qm.model.meshCount = qm.meshes.size();
  qm.model.meshes = (Mesh *)MemAlloc(qm.model.meshCount * sizeof(Mesh));
  qm.model.meshMaterial = (int *)MemAlloc(qm.model.meshCount * sizeof(int));

  for (int m = 0; m < qm.meshes.size(); m++) {
    qm.model.meshes[m] = qm.meshes[m];
    qm.model.meshMaterial[m] = qm.materialIDs[m];
  }

  return qm;
}
