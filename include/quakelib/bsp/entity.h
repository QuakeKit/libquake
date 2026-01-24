#pragma once

#include "bsp_file.h"
#include <functional>

namespace quakelib::bsp {
  class BaseEntity;
  class SolidEntity;

  typedef std::shared_ptr<BaseEntity> EntityPtr;

  enum EEntityType {
    ETypePontEntity = 0,
    ETypeSolidEntity = 1,
  };

  class BaseEntity {
  public:
    BaseEntity() {};
    virtual ~BaseEntity() = default;
    const string &Classname() const;
    const map<string, string> &Attributes();
    EEntityType Type() const;
    bool IsExternalModel() const;
    int ModelID() const;
    const vec3f_t &Origin();
    const float &Angle() const;
    static void ParseEntites(const char *entsrc, std::function<void(BaseEntity &ent)> f);

  protected:
    virtual void convertToOpenGLCoords();

    int m_modelId = 0;
    map<string, string> m_attributes;
    string m_classname = "";
    vec3f_t m_origin = {0};
    EEntityType m_type = ETypePontEntity;
    bool m_isExternalModel = false;
    float m_angle = 0;

  private:
    void setup();

    friend class QBsp;
  };
} // namespace quakelib::bsp