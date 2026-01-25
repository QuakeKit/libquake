#include <filesystem>
#include <quakelib/bsp/qbsp.h>
#include <quakelib/entity_parser.h>

namespace quakelib::bsp {
  template <typename T> bool loadLumptoVector(std::ifstream &istream, const lump_t &lump, vector<T> &v) {
    if (lump.length == 0) {
      return false;
    }
    v.resize(lump.length / sizeof(T));
    istream.seekg(lump.offset, istream.beg);
    istream.read((char *)(&v[0]), lump.length);
    return true;
  }

  int QBsp::LoadFile(const char *fileName) {
    m_istream.open(fileName, std::ios::binary);
    m_istream.read((char *)(&m_content.header), sizeof(header_t));

    if (m_content.header.version != MAGIC_V29 && m_content.header.version != MAGIC_V30) {
      return QBSP_ERR_WRONG_VERSION;
    }

    std::filesystem::path p = fileName;
    m_mapPath = string(p.replace_extension());

    loadLumptoVector(m_istream, m_content.header.lump[LUMP_VERTICES], m_content.vertices);
    loadLumptoVector(m_istream, m_content.header.lump[LUMP_EDGES], m_content.edges);
    loadLumptoVector(m_istream, m_content.header.lump[LUMP_FACES], m_content.faces);
    loadLumptoVector(m_istream, m_content.header.lump[LUMP_TEXINFO], m_content.surfaces);
    loadLumptoVector(m_istream, m_content.header.lump[LUMP_SURFEDGES], m_content.surfEdges);
    loadLumptoVector(m_istream, m_content.header.lump[LUMP_MODELS], m_content.models);

    loadLumptoVector(m_istream, m_content.header.lump[LUMP_PLANES], m_content.planes);
    loadLumptoVector(m_istream, m_content.header.lump[LUMP_NODES], m_content.nodes);
    loadLumptoVector(m_istream, m_content.header.lump[LUMP_LEAFS], m_content.leafs);

    if (m_config.loadTextures) {
      loadTextureInfo();
    }

    if (m_content.header.lump[LUMP_ENTITIES].length > 0) {
      m_istream.seekg(m_content.header.lump[LUMP_ENTITIES].offset, m_istream.beg);
      std::string entData;
      entData.resize(m_content.header.lump[LUMP_ENTITIES].length);
      m_istream.read(entData.data(), m_content.header.lump[LUMP_ENTITIES].length);

      EntityParser::ParseEntites(entData, [&](ParsedEntity *pe) {
        if (pe->type == EntityType::SOLID || pe->type == EntityType::WORLDSPAWN) {
          auto se = std::make_shared<SolidEntity>(this->Content(), pe);
          this->m_solidEntities.emplace_back(se);
          this->m_entities[se->ClassName()].push_back(se);
          if (se->IsWorldSpawn()) {
            this->m_worldSpawn = se;
          }
          return;
        }
        auto pt = std::make_shared<quakelib::PointEntity>();
        pt->FillFromParsed(pe);
        this->m_pointEntities.push_back(pt);
        this->m_entities[pt->ClassName()].emplace_back(pt);
      });
    }

    prepareLightMaps();
    prepareLevel();
    m_istream.close();

    return QBSP_OK;
  }

  void QBsp::prepareLightMaps() {
    int lm_size = m_content.header.lump[LUMP_LIGHTING].length;
    uint8_t *lm_dataBW = (uint8_t *)calloc(sizeof(uint8_t), lm_size);
    m_istream.seekg(m_content.header.lump[LUMP_LIGHTING].offset, m_istream.beg);
    m_istream.read((char *)lm_dataBW, m_content.header.lump[LUMP_LIGHTING].length);

    uint8_t *lm_dataRGB = nullptr;
    auto litFile = m_mapPath + ".lit";
    bool hasLitFile = false;
    if (std::filesystem::exists(litFile)) {
      auto length = std::filesystem::file_size(litFile);
      if (length > 0) {
        std::ifstream litStream(litFile, std::ios_base::binary);
        char magic[5] = {0};
        litStream.read(reinterpret_cast<char *>(&magic), 4);
        if (string(magic).compare("QLIT") == 0) {
          lm_dataRGB = (uint8_t *)calloc(sizeof(uint8_t), length - 4);
          litStream.read(reinterpret_cast<char *>(lm_dataRGB), length - 4);
          lm_dataRGB = lm_dataRGB + 4;
          lm_size = length - 8;
          hasLitFile = true;
        }
      }
    }
    if (!hasLitFile) {
      if (lm_dataRGB != nullptr) {
        free(lm_dataRGB);
      }
      lm_dataRGB = (uint8_t *)calloc(sizeof(uint8_t), lm_size * 3);
      int i2 = 0;
      for (int i = 0; i < lm_size; i++) {
        uint8_t d = lm_dataBW[i];
        lm_dataRGB[i2++] = d;
        lm_dataRGB[i2++] = d;
        lm_dataRGB[i2++] = d;
      }
      free(lm_dataBW);
    }

    m_lm = new Lightmap(lm_dataRGB, lm_size);
    m_lm->PackLitSurfaces(m_solidEntities);
  }

  void QBsp::prepareLevel() {
    if (m_config.convertCoordToOGL) {
      for (auto pe : m_entities) {
        for (auto &e : pe.second) {
          if (auto se = std::dynamic_pointer_cast<SolidEntity>(e)) {
            se->convertToOpenGLCoords();
          } else if (auto pt = std::dynamic_pointer_cast<quakelib::PointEntity>(e)) {
            auto o = pt->Origin();
            auto temp = o[1];
            o[1] = o[2];
            o[2] = -temp;
            pt->SetOrigin(o);
          }
        }
      }
    }
  }

  int QBsp::loadTextureInfo() {
    if (!m_istream.is_open() || m_content.header.version == 0) {
      return -1;
    }

    mipheader_t mh;
    m_istream.seekg(m_content.header.lump[LUMP_TEXTURES].offset, m_istream.beg);
    m_istream.read((char *)(&mh.numtex), sizeof(int32_t));
    mh.offset = (int32_t *)malloc(sizeof(int32_t) * mh.numtex);
    m_istream.read((char *)(mh.offset), sizeof(int32_t) * mh.numtex);

    m_content.miptextures.resize(mh.numtex);
    m_textures.resize(mh.numtex);
    for (int i = 0; i < mh.numtex; i++) {
      auto mho = mh.offset[i];
      if (mh.offset[i] < 0)
        continue;
      miptex_t miptex;
      auto t = mh.offset[i];
      m_istream.seekg(m_content.header.lump[LUMP_TEXTURES].offset + mh.offset[i], m_istream.beg);
      m_istream.read((char *)(&miptex), sizeof(miptex_t));
      m_content.miptextures[i] = miptex;
      bspTexure tex(miptex);
      if (m_config.loadTextureData) {
        auto texOffset = m_content.header.lump[LUMP_TEXTURES].offset + mh.offset[i] + miptex.offset[0];
        loadTexelBuff(&tex.data, texOffset, miptex.width * miptex.height);
        tex.hasData = true;
        tex.name = miptex.name;
      }
      m_textures[i] = tex;
    }

    return 0;
  }

  void QBsp::loadTexelBuff(unsigned char **buffOut, uint32_t offset, uint32_t len) {
    m_istream.seekg(offset, m_istream.beg);
    *buffOut = (unsigned char *)malloc(len);
    m_istream.read((char *)(*buffOut), len);
    return;
  }

  bool QBsp::Entities(const string &className, std::function<bool(EntityPtr)> cb) const {
    if (auto ev = m_entities.find(className); ev != m_entities.end()) {
      for (const auto e : m_entities.find(className)->second) {
        if (!cb(e))
          break;
      }
    }

    return false;
  }

  uint32_t QBsp::Version() const { return m_content.header.version; }

  const SolidEntityPtr QBsp::WorldSpawn() const { return m_worldSpawn; }

  const map<string, vector<EntityPtr>> &QBsp::Entities() const { return m_entities; }

  const vector<EntityPtr> &QBsp::PointEntities() const { return m_pointEntities; }

  const vector<SolidEntityPtr> &QBsp::SolidEntities() const { return m_solidEntities; }

  const SolidEntityPtr QBsp::ToSolidEntity(EntityPtr ent) {
    return std::dynamic_pointer_cast<SolidEntity>(ent);
  };

  const bspFileContent &QBsp::Content() const { return m_content; }

  const vector<bspTexure> &QBsp::Textures() const { return m_textures; };

  const Lightmap *QBsp::LightMap() const { return m_lm; };

} // namespace quakelib::bsp