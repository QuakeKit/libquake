#include <quakelib/bsp/entity.h>
#include <regex>
#include <sstream>

namespace quakelib::bsp {
  inline string &ltrim(string &s, const char *t = " ") {
    s.erase(0, s.find_first_not_of(t));
    return s;
  }

  static vector<string> rexec_vec(string line, const string &regexstr) {
    std::stringstream results;
    std::regex re(regexstr, std::regex::icase);

    vector<string> matches;
    const std::sregex_token_iterator end;
    for (std::sregex_token_iterator it(line.begin(), line.end(), re, 1); it != end; it++) {
      matches.push_back(*it);
    }

    return matches;
  }

  void BaseEntity::ParseEntites(const char *entsrc, std::function<void(BaseEntity &ent)> f) {
    auto entstr = std::stringstream(entsrc);
    int current_index = 0;
    auto current_ent = BaseEntity();

    for (string line; std::getline(entstr, line);) {
      std::erase(line, '\r');
      if (line.empty()) {
        continue;
      }
      if (line.starts_with("//")) {
        continue;
      }
      line = ltrim(line);
      if (line == "{") {
        current_ent = BaseEntity();
        continue;
      }

      if (line == "}") {
        current_ent.setup();

        f(current_ent);
        continue;
      }

      auto matches = rexec_vec(line, R"(\"([\s\S]*?)\")");
      if (matches.empty()) {
        continue;
      }

      current_ent.m_attributes.emplace(matches[0], matches[1]);
    }
  }

  void BaseEntity::setup() {
    if (m_attributes.contains("classname")) {
      m_classname = m_attributes["classname"];
      if (m_classname == "worldspawn") {
        this->m_type = ETypeSolidEntity;
        this->m_modelId = 0;
      }
    }

    if (m_attributes.contains("origin")) {
      std::stringstream stream(m_attributes["origin"]);
      stream >> m_origin.x >> m_origin.y >> m_origin.z;
      return;
    }

    if (m_attributes.contains("model")) {
      const auto &model = m_attributes["model"];
      if (model.starts_with("*")) {
        m_modelId = std::stoi(m_attributes["model"].c_str() + 1);
        m_type = ETypeSolidEntity;
        return;
      }
      m_isExternalModel = true;
      return;
    }

    if (m_attributes.contains("angle")) {

      std::stringstream stream(m_attributes["angle"]);
      stream >> m_angle;
      return;
    }
  }

  void BaseEntity::convertToOpenGLCoords() {
    auto temp = m_origin.y;
    m_origin.y = m_origin.z;
    m_origin.z = -temp;
    m_angle += 180;
  }

  const string &BaseEntity::Classname() const { return m_classname; };

  const map<string, string> &BaseEntity::Attributes() { return m_attributes; };

  EEntityType BaseEntity::Type() const { return m_type; };

  bool BaseEntity::IsExternalModel() const { return m_isExternalModel; };

  int BaseEntity::ModelID() const { return m_modelId; };

  const vec3f_t &BaseEntity::Origin() { return m_origin; };

  const float &BaseEntity::Angle() const { return m_angle; };
} // namespace quakelib::bsp