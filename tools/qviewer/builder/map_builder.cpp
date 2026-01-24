#include "map_builder.h"
#include "../common_utils.h"
#include <chrono>
#include <cstring>
#include <iostream>
#include <quakelib/map/lightmap_generator.h>
#include <quakelib/wad/wad.h>
#include <raymath.h>
#include <rlgl.h>

MapBuilder::MapBuilder(AssetManager &assetMgr, const QuakeMapOptions &opts)
    : assetMgr(assetMgr), opts(opts) {}

GeneratedMapData MapBuilder::Build(const std::string &filename) {
  GeneratedMapData data;
  using Clock = std::chrono::high_resolution_clock;
  using ms = std::chrono::milliseconds;

  auto t_parse_start = Clock::now();

  mapInstance = quakelib::map::QMap();
  std::cout << "Loading map file: " << filename << std::endl;
  mapInstance.LoadFile(filename, nullptr);

  // Load WADs specified in map (if any)
  if (mapInstance.HasWads()) {
    std::vector<std::string> wadFiles;
    for (const auto &wf : mapInstance.Wads()) {
      wadFiles.push_back(wf);
    }
    assetMgr.LoadWads(wadFiles);
  }

  // Register texture bounds provider using AssetManager
  mapInstance.RegisterTextureBounds([this](const char *name) -> quakelib::map::textureBounds {
    std::string sName = to_lower(name);
    auto qt = this->assetMgr.GetWadManager().FindTexture(sName);
    if (qt) {
      return quakelib::map::textureBounds{(float)qt->width, (float)qt->height};
    }
    return quakelib::map::textureBounds{0, 0};
  });

  // Special texture types
  mapInstance.SetFaceTypeByTextureID("CLIP", quakelib::map::MapSurface::CLIP);
  mapInstance.SetFaceTypeByTextureID("TRIGGER", quakelib::map::MapSurface::CLIP);
  mapInstance.SetFaceTypeByTextureID("clip", quakelib::map::MapSurface::CLIP);
  mapInstance.SetFaceTypeByTextureID("trigger", quakelib::map::MapSurface::CLIP);

  auto t_parse_end = Clock::now();
  auto t_geo_start = Clock::now();

  std::cout << "Generating Geometry..." << std::endl;
  mapInstance.GenerateGeometry(true);

  auto t_geo_end = Clock::now();
  auto t_lm_start = Clock::now();

  // Lightmap Generation
  data.lightmapAtlas = generateLightmapAtlas();

  auto t_lm_end = Clock::now();

  // Stats
  long long t_lm = std::chrono::duration_cast<ms>(t_lm_end - t_lm_start).count();
  long long t_parse = std::chrono::duration_cast<ms>(t_parse_end - t_parse_start).count();
  long long t_geo = std::chrono::duration_cast<ms>(t_geo_end - t_geo_start).count();

  std::cout << "--------------------------------\n"
            << "Map Compilation Stats:\n"
            << " Parsing:       " << t_parse << " ms\n"
            << " Geometry Gen:  " << t_geo << " ms\n"
            << " Lightmap Bake: " << t_lm << " ms\n" // This includes atlas texture upload
            << "--------------------------------" << std::endl;

  // Convert entities to models
  for (const auto &se : mapInstance.SolidEntities()) {
    auto m = readModelEntity(se);
    // Only add if it has meshes
    if (m.model.meshCount > 0) {
      data.models.push_back(m);
    }
  }

  // Player Start
  auto startEnts = mapInstance.PointEntitiesByClass("info_player_start");
  if (!startEnts.empty()) {
    data.playerStart = startEnts[0];
  } else {
    data.playerStart = nullptr;
  }

  data.textureNames = mapInstance.TextureNames();

  return data;
}

Texture2D MapBuilder::generateLightmapAtlas() {
  quakelib::map::LightmapGenerator lmGen(2048, 2048);
  std::vector<quakelib::map::SolidEntityPtr> allSolids;
  for (const auto &se : mapInstance.SolidEntities()) {
    allSolids.push_back(se);
  }

  if (!lmGen.Pack(allSolids)) {
    std::cerr << "Failed to pack lightmaps! Using default white." << std::endl;
    Image whiteImg = GenImageColor(4, 4, WHITE);
    Texture2D tex = LoadTextureFromImage(whiteImg);
    UnloadImage(whiteImg);
    return tex;
  }

  std::cout << "Lightmap Atlas Generated: " << lmGen.GetWidth() << "x" << lmGen.GetHeight() << std::endl;

  std::vector<quakelib::map::LightmapGenerator::Light> lights;

  // Player Start Light (Flashlight?)
  auto startEnts = mapInstance.PointEntitiesByClass("info_player_start");
  if (!startEnts.empty()) {
    fvec3 pos = startEnts[0]->Origin();
    pos[2] += 64.0f;
    lights.push_back({pos, 300.0f, {1.0f, 1.0f, 1.0f}});
  }

  // Entity Lights
  auto lightEnts = mapInstance.PointEntitiesByClass("light");
  for (const auto &ent : lightEnts) {
    float r = ent->AttributeFloat("light");
    if (r <= 1.0f)
      r = 300.0f;

    fvec3 color = {1.0f, 1.0f, 1.0f};
    if (ent->Attributes().count("_color")) {
      color = ent->AttributeVec3("_color");
    } else if (ent->Attributes().count("color")) {
      color = ent->AttributeVec3("color");
    }

    // Normalize color if it's 0-255
    if (color[0] > 1.0f || color[1] > 1.0f || color[2] > 1.0f) {
      color[0] /= 255.0f;
      color[1] /= 255.0f;
      color[2] /= 255.0f;
    }

    lights.push_back({ent->Origin(), r, color});
  }

  fvec3 ambient = {0.3f, 0.3f, 0.3f};
  auto worldspawn = mapInstance.SolidEntities().empty() ? nullptr : mapInstance.SolidEntities()[0];
  if (worldspawn && worldspawn->ClassName() == "worldspawn") {
    if (worldspawn->Attributes().count("ambient")) {
      float a = worldspawn->AttributeFloat("ambient");
      ambient = {a / 255.0f, a / 255.0f, a / 255.0f};
    }
  }

  std::cout << "Baking " << lights.size() << " lights..." << std::endl;
  lmGen.CalculateLighting(lights, ambient);

  Image atlasImg;
  atlasImg.data = (void *)lmGen.GetAtlasData().data();
  atlasImg.width = lmGen.GetWidth();
  atlasImg.height = lmGen.GetHeight();
  atlasImg.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
  atlasImg.mipmaps = 1;

  Texture2D lightmapAtlas = LoadTextureFromImage(atlasImg);
  SetTextureFilter(lightmapAtlas, TEXTURE_FILTER_BILINEAR);
  return lightmapAtlas;
}

QuakeModel MapBuilder::readModelEntity(const quakelib::map::SolidEntityPtr &ent) {
  QuakeModel qm;
  std::map<int, std::vector<quakelib::map::FacePtr>> batchedFaces;

  for (const auto &b : ent->GetClippedBrushes()) {
    for (const auto &p : b.Faces()) {
      if (p->Type() == quakelib::map::MapSurface::CLIP) {
        continue;
      }
      batchedFaces[p->TextureID()].push_back(p);
    }
  }

  for (auto const &[texID, faces] : batchedFaces) {
    if (faces.empty())
      continue;

    int totalTriangles = 0;
    for (const auto &f : faces) {
      totalTriangles += f->Indices().size() / 3;
    }

    if (totalTriangles == 0)
      continue;

    auto mesh = Mesh{0};
    mesh.triangleCount = totalTriangles;
    mesh.vertexCount = totalTriangles * 3;

    mesh.vertices = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.tangents = (float *)MemAlloc(mesh.vertexCount * 4 * sizeof(float));
    mesh.texcoords = (float *)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
    mesh.texcoords2 = (float *)MemAlloc(mesh.vertexCount * 2 * sizeof(float));
    mesh.normals = (float *)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.colors = (unsigned char *)MemAlloc(mesh.vertexCount * 4 * sizeof(unsigned char));
    mesh.indices = nullptr;

    int vOffset = 0;
    unsigned char baryColors[3][4] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};

    for (const auto &p : faces) {
      auto vertices = p->Vertices();
      auto indices = p->Indices();

      for (int k = 0; k < indices.size(); k += 3) {
        int idx[3] = {(int)indices[k + 2], (int)indices[k + 1], (int)indices[k]};

        for (int vIter = 0; vIter < 3; vIter++) {
          int vIndex = idx[vIter];
          int currentVertex = vOffset + vIter;

          // Note inverted coordinate system handling (Quake Z-up vs Raylib Y-up usually)
          // Original: x = -x, y = z, z = y. Rotation happening here or via camera?
          // Previous code: x = -origin.x, y = origin.z, z = origin.y
          mesh.vertices[currentVertex * 3 + 0] = -vertices[vIndex].point[0] / opts.inverseScale;
          mesh.vertices[currentVertex * 3 + 1] = vertices[vIndex].point[2] / opts.inverseScale;
          mesh.vertices[currentVertex * 3 + 2] = vertices[vIndex].point[1] / opts.inverseScale;

          mesh.normals[currentVertex * 3 + 0] = -vertices[vIndex].normal[0];
          mesh.normals[currentVertex * 3 + 1] = vertices[vIndex].normal[2];
          mesh.normals[currentVertex * 3 + 2] = vertices[vIndex].normal[1];

          mesh.tangents[currentVertex * 4 + 0] = -vertices[vIndex].tangent[0];
          mesh.tangents[currentVertex * 4 + 1] = vertices[vIndex].tangent[2];
          mesh.tangents[currentVertex * 4 + 2] = vertices[vIndex].tangent[1];
          mesh.tangents[currentVertex * 4 + 3] = 1.0f;

          mesh.texcoords[currentVertex * 2 + 0] = vertices[vIndex].uv[0];
          mesh.texcoords[currentVertex * 2 + 1] = vertices[vIndex].uv[1];

          mesh.texcoords2[currentVertex * 2 + 0] = vertices[vIndex].lightmap_uv[0];
          mesh.texcoords2[currentVertex * 2 + 1] = vertices[vIndex].lightmap_uv[1];

          mesh.colors[currentVertex * 4 + 0] = baryColors[vIter][0];
          mesh.colors[currentVertex * 4 + 1] = baryColors[vIter][1];
          mesh.colors[currentVertex * 4 + 2] = baryColors[vIter][2];
          mesh.colors[currentVertex * 4 + 3] = baryColors[vIter][3];
        }
        vOffset += 3;
      }
    }

    UploadMesh(&mesh, false);
    qm.meshes.push_back(mesh);
    qm.materialIDs.push_back(texID);
  }

  qm.model.transform = MatrixIdentity();
  qm.model.meshCount = qm.meshes.size();
  // We can't set materialCount/materials yet as that's handled by AssetManager/Renderer coupling
  // But we need to allocate the mesh array
  qm.model.meshes = (Mesh *)MemAlloc(qm.model.meshCount * sizeof(Mesh));
  qm.model.meshMaterial = (int *)MemAlloc(qm.model.meshCount * sizeof(int));

  for (int m = 0; m < qm.meshes.size(); m++) {
    qm.model.meshes[m] = qm.meshes[m];
    // Map the local mesh material index to the global Map texture index
    qm.model.meshMaterial[m] = qm.materialIDs[m];
  }

  return qm;
}
