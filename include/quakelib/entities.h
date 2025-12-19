#pragma once

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <quakelib/surface.h>
#include <quakelib/tue/vec.hpp>

namespace quakelib {

  enum class EntityType {
    POINT = 0,
    SOLID = 1,
    WORLDSPAWN = 2,
  };

  using attribMap_t = std::map<std::string, std::string>;

  struct ParsedEntity {
    std::stringstream lines;
    ParsedEntity *parent = nullptr;
    std::vector<ParsedEntity *> children;
    EntityType type = EntityType::POINT;
  };

  class Entity {
  public:
    explicit Entity(EntityType type) : m_type(type) {};
    virtual void FillFromParsed(ParsedEntity *pe);
    [[nodiscard]] const std::string &ClassName() const;
    [[nodiscard]] bool ClassContains(const std::string &substr) const;

    // Attribute Methods
    const attribMap_t &Attributes() const;
    std::string AttributeStr(const std::string &classname);
    float AttributeFloat(const std::string &classname);
    tue::fvec3 AttributeVec3(const std::string &classname);
    tue::fvec2 AttributsVec2(const std::string &classname);

  protected:
    attribMap_t m_attributes;
    std::string m_classname;
    std::string m_tbName;
    std::string m_tbType;
    EntityType m_type = EntityType::POINT;
  };

  class PointEntity : public Entity {
  public:
    PointEntity() : Entity(EntityType::POINT) {};
    virtual void FillFromParsed(ParsedEntity *pe);
    const tue::fvec3 &Origin() const;
    float Angle();

  private:
    tue::fvec3 m_origin{0};
    float m_angle = 0;
  };

  class SolidEntity : public Entity {
  public:
    SolidEntity() : Entity(EntityType::SOLID) {};
    virtual void FillFromParsed(ParsedEntity *pe);

  protected:
    std::vector<Surface> m_surfaces;
    bool m_hasPhongShading = false;
  };

  class WorldSpawnEntity : public SolidEntity {
  public:
    virtual void FillFromParsed(ParsedEntity *pe);

  private:
    std::vector<std::string> m_wads;
  };
} // namespace quakelib