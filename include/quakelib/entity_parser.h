#pragma once

#include <functional>
#include <quakelib/entities.h>
#include <sstream>

namespace quakelib {

  using EntityParsedFunc = std::function<void(ParsedEntity *)>;

  class EntityParser {

  public:
    static void ParseEntites(const std::string &buffer, EntityParsedFunc fn);
    static void ParseEntites(std::istream &stream, EntityParsedFunc fn);
  };

} // namespace quakelib