#pragma once

#include <functional>
#include <quakelib/entities.h>
#include <sstream>

namespace quakelib {

  /**
   * @brief Callback function type for handling parsed entities.
   * @param[in] parsed_entity Pointer to the newly parsed entity structure.
   */
  using EntityParsedFunc = std::function<void(ParsedEntity *)>;

  /**
   * @brief Provides functionality to parse entity data from streams or strings.
   */
  class EntityParser {

  public:
    /**
     * @brief Parses entities from a string buffer.
     *
     * Iterates through the provided string, parsing each entity found and
     * invoking the callback function.
     *
     * @param buffer The string containing the entity data to parse.
     * @param fn The callback function to execute for each parsed entity.
     */
    static void ParseEntites(const std::string &buffer, EntityParsedFunc fn);

    /**
     * @brief Parses entities from an input stream.
     *
     * Iterates through the input stream, parsing each entity found and
     * invoking the callback function.
     *
     * @param stream The input stream to read entity data from.
     * @param fn The callback function to execute for each parsed entity.
     */
    static void ParseEntites(std::istream &stream, EntityParsedFunc fn);
  };

} // namespace quakelib