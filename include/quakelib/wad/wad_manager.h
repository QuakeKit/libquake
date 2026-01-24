#pragma once

#include <map>
#include <memory>
#include <quakelib/wad/wad.h>
#include <string>
#include <vector>

namespace quakelib::wad {

  /**
   * @brief Manages multiple WAD files and provides unified texture lookup.
   *
   * This class allows loading multiple WAD files (e.g., from a map's wad list)
   * and searching for a texture across all of them in order.
   */
  class QuakeWadManager {
  public:
    /**
     * @brief Default constructor.
     */
    QuakeWadManager();

    /**
     * @brief Destructor.
     */
    ~QuakeWadManager();

    /**
     * @brief Adds a WAD file to the manager.
     * @param path The file path to the .wad file.
     */
    void AddWadFile(const std::string &path);

    /**
     * @brief Finds a texture by name across all loaded WADs.
     *
     * Search order is typically the order in which WADs were added.
     *
     * @param name The name of the texture to find.
     * @return Pointer to the texture if found, nullptr otherwise.
     */
    QuakeTexture *FindTexture(const std::string &name);

  private:
    std::vector<QuakeWadPtr> wads; ///< List of managed WAD files.
  };

} // namespace quakelib::wad
