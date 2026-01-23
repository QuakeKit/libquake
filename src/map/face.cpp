#include <iostream>
#include <quakelib/map/face.h>

namespace quakelib::map {
  static constexpr double CMP_EPSILON_DISTANCE = 0.001;

  void MapSurface::initPlane() {
    fvec3 v0v1 = m_planePoints[1] - m_planePoints[0];
    fvec3 v1v2 = m_planePoints[2] - m_planePoints[1];
    m_planeNormal = normalize(cross(v1v2, v0v1));
    m_planeDist = dot(m_planeNormal, m_planePoints[0]);
  }

  FacePtr MapSurface::Copy() const {
    auto newp = std::make_shared<MapSurface>();
    newp->m_vertices = m_vertices;
    newp->m_indices = m_indices;
    newp->m_planeNormal = m_planeNormal;
    newp->m_planeDist = m_planeDist;
    newp->m_valveUV = m_valveUV;
    newp->m_scaleY = m_scaleY;
    newp->m_scaleX = m_scaleX;
    newp->m_hasValveUV = m_hasValveUV;
    newp->min = min;
    newp->max = max;
    return newp;
  }

  void MapSurface::UpdateNormals() {
    for (int i = 0; i < m_indices.size(); i += 3) {
      // get the three vertices that make the faces
      const auto &p1 = m_vertices[m_indices[i + 0]].point;
      const auto &p2 = m_vertices[m_indices[i + 1]].point;
      const auto &p3 = m_vertices[m_indices[i + 2]].point;

      auto v1 = p2 - p1;
      auto v2 = p3 - p1;
      auto normal = normalize(cross(v1, v2));

      m_vertices[m_indices[i + 0]].normal = normal;
      m_vertices[m_indices[i + 1]].normal = normal;
      m_vertices[m_indices[i + 2]].normal = normal;
    }
  }

  MapSurface::eFaceClassification MapSurface::Classify(const MapSurface *other) {
    bool bFront = false, bBack = false;
    for (int i = 0; i < (int)other->m_vertices.size(); i++) {
      double dist = dot(m_planeNormal, other->m_vertices[i].point) - m_planeDist;
      if (dist > CMP_EPSILON_DISTANCE) {
        if (bBack) {
          return eFaceClassification::SPANNING;
        }

        bFront = true;
      } else if (dist < -CMP_EPSILON_DISTANCE) {
        if (bFront) {
          return eFaceClassification::SPANNING;
        }

        bBack = true;
      }
    }

    if (bFront) {
      return eFaceClassification::FRONT;
    } else if (bBack) {
      return eFaceClassification::BACK;
    }

    return eFaceClassification::ON_PLANE;
  }

  bool MapSurface::getIntersection(const fvec3 &start, const fvec3 &end, fvec3 &out_intersectionPt,
                                   float &out_percentage) {
    fvec3 dir = normalize(end - start);
    float num, denom;

    denom = dot(m_planeNormal, dir);

    if (fabs(denom) < epsilon) {
      return false;
    }

    float dist = tue::math::dot(m_planeNormal, start) - m_planeDist;

    num = -dist;
    out_percentage = num / denom;
    out_intersectionPt = start + (dir * out_percentage);
    out_percentage = out_percentage / tue::math::length(end - start);
    return true;
  }

  MapSurface::eFaceClassification MapSurface::ClassifyPoint(const fvec3 &v) {
    double dist = tue::math::dot(m_planeNormal, v) - m_planeDist;
    if (dist > epsilon) {
      return MapSurface::eFaceClassification::FRONT;
    }
    if (dist < -epsilon) {
      return MapSurface::eFaceClassification::BACK;
    }
    return MapSurface::eFaceClassification::ON_PLANE;
  }

  void MapSurface::UpdateAB() {
    if (m_vertices.empty())
      return;

    min = m_vertices[0].point;
    max = m_vertices[0].point;

    for (const auto &vert : m_vertices) {
      if (vert.point[0] < min[0])
        min[0] = vert.point[0];

      if (vert.point[1] < min[1])
        min[1] = vert.point[1];

      if (vert.point[2] < min[2])
        min[2] = vert.point[2];

      if (vert.point[0] > max[0])
        max[0] = vert.point[0];

      if (vert.point[1] > max[1])
        max[1] = vert.point[1];

      if (vert.point[2] > max[2])
        max[2] = vert.point[2];
    }
  }

  bool MapSurface::operator==(const MapSurface &other) const {
    if (m_vertices.size() != other.m_vertices.size() || m_planeDist != other.m_planeDist ||
        m_planeNormal != other.m_planeNormal)
      return false;

    for (int i = 0; i < m_vertices.size(); i++) {
      if (m_vertices[i].point != other.m_vertices[i].point)
        return false;

      if (m_vertices[i].uv[0] != other.m_vertices[i].uv[0])
        return false;

      if (m_vertices[i].uv[1] != other.m_vertices[i].uv[1])
        return false;
    }

    if (m_textureID == other.m_textureID)
      return true;

    return true;
  }

  fvec4 MapSurface::calcStandardTangent() {
    float du = dot(m_planeNormal, UP_VEC);
    float dr = dot(m_planeNormal, RIGHT_VEC);
    float df = dot(m_planeNormal, FORWARD_VEC);
    float dua = fabs(du);
    float dra = fabs(dr);
    float dfa = fabs(df);

    fvec3 uAxis{0};
    float vSign = 0.0f;

    if (dua >= dra && dua >= dfa) {
      uAxis = FORWARD_VEC;
      vSign = copysignf(1.0, du);
    } else if (dra >= dua && dra >= dfa) {
      uAxis = FORWARD_VEC;
      vSign = -copysignf(1.0, dr);
    } else if (dfa >= dua && dfa >= dra) {
      uAxis = RIGHT_VEC;
      vSign = copysignf(1.0, df);
    }

    vSign *= copysignf(1.0, m_scaleY);
    uAxis = tue::transform::rotation_vec(uAxis, (float)((-m_rotation * vSign) * (180.0 / M_PI)));
    return fvec4(uAxis[0], uAxis[1], uAxis[2], vSign);
  }

  fvec4 MapSurface::calcValveTangent() {
    fvec3 uAxis = normalize(m_valveUV.u.xyz());
    fvec3 vAxis = normalize(m_valveUV.v.xyz());
    float vSign = copysignf(1.0, dot(cross((fvec3)m_planeNormal, uAxis), vAxis));
    return fvec4(uAxis[0], uAxis[1], uAxis[2], vSign);
  }

  fvec2 MapSurface::calcStandardUV(fvec3 vertex, float texW, float texH) {
    fvec2 uvOut{0};

    float du = fabs(dot(m_planeNormal, UP_VEC));
    float dr = fabs(dot(m_planeNormal, RIGHT_VEC));
    float df = fabs(dot(m_planeNormal, FORWARD_VEC));

    if (du >= dr && du >= df)
      uvOut = fvec2(vertex[0], -vertex[1]);
    else if (dr >= du && dr >= df)
      uvOut = fvec2(vertex[0], -vertex[2]);
    else if (df >= du && df >= dr)
      uvOut = fvec2(vertex[1], -vertex[2]);

    float angle = (float)(m_rotation * (M_PI / 180));
    uvOut =
        fvec2(uvOut[0] * cos(angle) - uvOut[1] * sin(angle), uvOut[0] * sin(angle) + uvOut[1] * cos(angle));

    uvOut[0] /= texW;
    uvOut[1] /= texH;

    uvOut[0] /= m_scaleX;
    uvOut[1] /= m_scaleY;

    uvOut[0] += m_standardUV.u / texW;
    uvOut[1] += m_standardUV.v / texH;
    return uvOut;
  }

  fvec2 MapSurface::calcValveUV(fvec3 vertex, float texW, float texH) {
    fvec2 uvOut{0};
    fvec3 uAxis = m_valveUV.u.xyz();
    fvec3 vAxis = m_valveUV.v.xyz();
    float uShift = m_valveUV.u[3];
    float vShift = m_valveUV.v[3];

    uvOut[0] = dot(uAxis, vertex);
    uvOut[1] = dot(vAxis, vertex);

    uvOut[0] /= texW;
    uvOut[1] /= texH;

    uvOut[0] /= m_scaleX;
    uvOut[1] /= m_scaleY;

    uvOut[0] += uShift / texW;
    uvOut[1] += vShift / texH;

    return uvOut;
  }

  std::pair<FacePtr, FacePtr> MapSurface::splitFace(const MapSurface *other) {
    std::vector<MapSurface::eFaceClassification> pCF;
    pCF.resize(other->m_vertices.size());
    for (int i = 0; i < other->m_vertices.size(); i++)
      pCF[i] = ClassifyPoint(other->m_vertices[i].point);

    FacePtr front = std::make_shared<MapSurface>();
    FacePtr back = std::make_shared<MapSurface>();

    for (int i = 0; i < other->m_vertices.size(); i++) {
      switch (pCF[i]) {
      case FRONT: {
        front->m_vertices.push_back(other->m_vertices[i]);
        break;
      }
      case BACK: {
        back->m_vertices.push_back(other->m_vertices[i]);
        break;
      }
      case ON_PLANE:
        std::cout << "on plane" << std::endl;
        {
          front->m_vertices.push_back(other->m_vertices[i]);
          back->m_vertices.push_back(other->m_vertices[i]);
          break;
        }
      case SPANNING:
        break;
      }

      int j = i + 1;
      bool ignore = false;

      if (j == (other->m_vertices.size() - 1))
        j = 0;

      if ((pCF[i] == MapSurface::eFaceClassification::ON_PLANE) &&
          (pCF[j] != MapSurface::eFaceClassification::ON_PLANE)) {
        ignore = true;
      } else if ((pCF[j] == MapSurface::eFaceClassification::ON_PLANE) &&
                 (pCF[i] != MapSurface::eFaceClassification::ON_PLANE)) {
        ignore = true;
      }

      if ((!ignore) && (pCF[i] != pCF[j])) {
        Vertex v{};
        float p;
        getIntersection(other->m_vertices[i].point, other->m_vertices[j].point, v.point, p);

        v.uv[0] = other->m_vertices[j].uv[0] - other->m_vertices[i].uv[0];
        v.uv[1] = other->m_vertices[j].uv[1] - other->m_vertices[i].uv[1];

        v.uv[0] = other->m_vertices[i].uv[0] + (p * v.uv[0]);
        v.uv[1] = other->m_vertices[i].uv[1] + (p * v.uv[1]);

        front->m_vertices.push_back(v);
        back->m_vertices.push_back(v);
      }
    }
    return {front, back};
  }
} // namespace quakelib::map