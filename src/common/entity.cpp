#include <quakelib/entities.h>

#include <iostream>
#include <regex>
#include <sstream>
#include <string>

namespace quakelib {

  using std::string;

  const char *STR_CLASSNAME = "classname";




  const string &Entity::ClassName() const { return m_classname; }

  bool Entity::ClassContains(const std::string &substr) const {
    return m_classname.find(substr) != std::string::npos;
  };

  const attribMap_t &Entity::Attributes() const { return m_attributes; };

  std::string Entity::AttributeStr(const std::string &classname) {
    auto val = m_attributes.find(classname);
    if (m_attributes.find(classname) != m_attributes.end()) {
      return val->second;
    }
    return "";
  }

  float Entity::AttributeFloat(const std::string &classname) {
    auto val = AttributeStr(classname);
    if (val == "")
      return 0;

    return std::atof(val.c_str());
  }

  tue::fvec3 Entity::AttributeVec3(const std::string &classname) {
    auto ret = tue::fvec3{};
    auto val = AttributeStr(classname);
    if (val == "")
      return ret;

    std::stringstream stream(val);
    stream >> ret[0] >> ret[1] >> ret[2];
    return ret;
  }

  tue::fvec2 Entity::AttributsVec2(const std::string &classname) {
    auto ret = tue::fvec2{};
    auto val = AttributeStr(classname);
    if (val == "")
      return ret;

    std::stringstream stream(val);
    stream >> ret[0] >> ret[1];
    return ret;
  }




  const tue::fvec3 &PointEntity::Origin() const { return m_origin; }

  float PointEntity::Angle() { return m_angle; }

}