#pragma once

#include "bsp_file.h"
#include "entity_solid.h"
#include "lightmap.h"
#include "primitives.h"
#include <quakelib/entities.h>

#include <fstream>
#include <map>

namespace quakelib::bsp {
  using EntityPtr = std::shared_ptr<quakelib::Entity>;

  enum EQBspStatus {
    QBSP_OK = 0,
    QBSP_ERR_WRONG_VERSION = -1001,
  };

  struct bspTexure {
    bspTexure() = default;

    bspTexure(const miptex_t &mt) {
      width = mt.width;
      height = mt.height;
      name = string(mt.name);
    };

    std::string name;
    uint32_t id;
    uint32_t width;
    uint32_t height;
    bool hasData;
    unsigned char *data;
  };

  struct QBspConfig {
    // load texture lump.
    bool loadTextures = true;
    // also load texture data when loading texture lump.
    bool loadTextureData = true;
    // convert coordinates to OpenGL;
    bool convertCoordToOGL = false;
  };

  /**
   *  QBsp
   *  Structure of a BSP file.
   */
  class QBsp {
  public:
    QBsp() = default;
    QBsp(QBspConfig cfg) : m_config(cfg) {};
    ~QBsp() = default;
    int LoadFile(const char *filename);
    uint32_t Version() const;

    const SolidEntityPtr WorldSpawn() const;
    const std::map<string, vector<EntityPtr>> &Entities() const;
    bool Entities(const string &className, std::function<bool(EntityPtr)> cb) const;
    const vector<EntityPtr> &PointEntities() const;
    const vector<SolidEntityPtr> &SolidEntities() const;

    static const SolidEntityPtr ToSolidEntity(EntityPtr ent);
    const bspFileContent &Content() const;
    const vector<bspTexure> &Textures() const;
    const Lightmap *LightMap() const;

  private:
    void parseEntities(const char *entsrc);
    int loadTextureInfo();
    void prepareLevel();
    void prepareLightMaps();
    void loadTexelBuff(unsigned char **buffOut, uint32_t offset, uint32_t len);

    std::ifstream m_istream;
    QBspConfig m_config;
    string m_mapPath = "";

    vector<EntityPtr> m_pointEntities;
    std::map<string, vector<EntityPtr>> m_entities;
    vector<SolidEntityPtr> m_solidEntities;

    vector<bspTexure> m_textures;
    SolidEntityPtr m_worldSpawn;

    bspFileContent m_content;
    Lightmap *m_lm;
  };
} // namespace quakelib::bsp