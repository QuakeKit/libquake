#include <iostream>
#include <quakelib/wad/wad_manager.h>

namespace quakelib::wad {

  QuakeWadManager::QuakeWadManager() {}

  QuakeWadManager::~QuakeWadManager() {}

  void QuakeWadManager::AddWadFile(const std::string &path) {
    try {
      auto wad = QuakeWad::FromFile(path);
      if (wad) {
        wads.push_back(wad);
      }
    } catch (const std::exception &e) {
      std::cerr << "Failed to load WAD " << path << ": " << e.what() << std::endl;
    }
  }

  QuakeTexture *QuakeWadManager::FindTexture(const std::string &name) {
    for (auto &wad : wads) {
      auto tex = wad->GetTexture(name);
      if (tex) {
        return tex;
      }
    }
    return nullptr;
  }

}
