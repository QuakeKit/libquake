#pragma once
#include <ctype.h>
#include <string>

inline std::string to_lower(std::string s) {
  for (char &c : s)
    c = tolower(c);
  return s;
}
