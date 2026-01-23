#pragma once

#include "palette.h"
#include <fstream>
#include <map>
#include <memory>
#include <string>

namespace quakelib::wad {
  static const int TEXTURE_NAME_LENGTH = 16;
  static const int MAX_MIP_LEVELS = 4;

  enum QuakeTextureType { TTYPE_DEFAULT_TEXTURE = 0, TTYPE_SKY_TEXTURE = 1 };

  struct QuakeTexture {
    virtual void FillTextureData(const uint8_t *buff, size_t size, bool flipHorizontal, const Palette &pal);
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int mipOffsets[MAX_MIP_LEVELS];
    QuakeTextureType type = TTYPE_DEFAULT_TEXTURE;
    cvec raw;
  };

  struct QuakeSkyTexture : public QuakeTexture {
    void FillTextureData(const uint8_t *buff, size_t size, bool flipHorizontal, const Palette &pal);
    const cvec &BaseSky() const;
    const cvec &Alphaky() const;
    cvec rawFront;
  };

  struct QuakeWadEntry {
    enum QuakeWadEntryType {
      QWET_Palette = 0x40,
      QWET_SBarPic = 0x42,
      QWET_MipsTexture = 0x44,
      QWET_ConsolePic = 0x45
    };

    struct header {
      unsigned int offset;
      unsigned int inWadSize;
      unsigned int size;
      unsigned char type;
      unsigned char compression;
      unsigned short unknown;
    } header;

    std::string name;
    QuakeTexture texture;

    QuakeWadEntryType Type() { return (QuakeWadEntryType)header.type; }
  };

  class QuakeWad;
  using QuakeWadPtr = std::shared_ptr<QuakeWad>;

  struct QuakeWadOptions {
    bool flipTexHorizontal;
  };

  class QuakeWad {
  public:
    QuakeWadOptions opts;

  public:
    static QuakeWadPtr FromFile(const std::string &fileName, QuakeWadOptions opts = QuakeWadOptions());

    static QuakeWadPtr NewQuakeWad() { return std::make_shared<QuakeWad>(); }

    QuakeTexture *FromBuffer(const uint8_t *buff, bool isSky, int width, int height);
    ~QuakeWad();
    QuakeTexture *GetTexture(const std::string &textureName);

    const std::map<std::string, QuakeWadEntry> &Textures() { return m_entries; };

    void SetPalette(const Palette &p) { this->m_pal = p; };

    const Palette &GetPalette() { return m_pal; };

    static bool IsSkyTexture(const std::string texname);

  private:
    unsigned int m_numEntries;
    unsigned int m_dirOffset;
    Palette m_pal = default_palette;
    std::ifstream m_istream;
    std::map<std::string, QuakeWadEntry> m_entries;
  };
} // namespace quakelib::wad