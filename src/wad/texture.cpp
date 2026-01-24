#include <quakelib/wad/wad.h>

namespace quakelib::wad {
  void QuakeTexture::FillTextureData(const uint8_t *buff, size_t size, bool flipHorizontal,
                                     const Palette &pal) {
    int k = 0, w = 0;
    int h = flipHorizontal ? height - 1 : 0;
    raw = cvec(size);
    for (int idx = size - 1; idx >= 0; idx--) {
      auto rgba = pal.GetColor((int)(buff[w + (h * width)]));
      raw[k] = rgba;
      k++, w++;
      if (w == width) {
        w = 0;
        flipHorizontal ? h-- : h++;
      }
    }
    return;
  }

  void QuakeSkyTexture::FillTextureData(const uint8_t *buff, size_t size, bool flipHorizontal,
                                        const Palette &pal) {
    width /= 2;
    type = wad::TTYPE_SKY_TEXTURE;
    int k = 0, w = 0;
    int h = flipHorizontal ? height - 1 : 0;
    rawFront = cvec(size);
    for (int idx = size - 1; idx >= 0; idx--) {
      auto rgba = pal.GetColor((int)(buff[w + (h * (width + width))]));
      rawFront[k] = rgba;
      k++, w++;
      if (w == width) {
        w = 0;
        flipHorizontal ? h-- : h++;
        if (h < 0) {
          h = 0;
        }
      }
    }

    k = 0, w = width;
    h = flipHorizontal ? height - 1 : 0;
    raw = cvec(size);
    for (int idx = size - 1; idx >= 0; idx--) {
      auto rgba = pal.GetColor((int)(buff[w + (h * (width + width))]));
      raw[k] = rgba;
      k++, w++;
      if (w == (width + width)) {
        w = width;
        flipHorizontal ? h-- : h++;
        if (h < 0) {
          h = 0;
        }
      }
    }
    return;
  }

  const cvec &QuakeSkyTexture::BaseSky() const { return raw; }

  const cvec &QuakeSkyTexture::Alphaky() const { return rawFront; }
}