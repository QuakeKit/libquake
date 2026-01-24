#include <iostream>
#include <quakelib/map/brush.h>
#include <quakelib/map/qmath.h>

namespace quakelib::map {
  const auto fv3zero = Vertex();

  void Brush::buildGeometry(const std::map<int, MapSurface::eFaceType> &faceTypes,
                            const std::map<int, textureBounds> &texBounds) {
    generatePolygons(faceTypes, texBounds);
    windFaceVertices();
    indexFaceVertices();
    calculateAABB();
  }

  void Brush::GetBiggerBBox(fvec3 &outMin, fvec3 &outMax) {
    outMax[0] = max[0] > outMax[0] ? max[0] : outMax[0];
    outMax[1] = max[1] > outMax[1] ? max[1] : outMax[1];
    outMax[2] = max[2] > outMax[2] ? max[2] : outMax[2];

    outMin[0] = min[0] < outMin[0] ? min[0] : outMin[0];
    outMin[1] = min[1] < outMin[1] ? min[1] : outMin[1];
    outMin[2] = min[2] < outMin[2] ? min[2] : outMin[2];
  }

  boolRet<Vertex> Brush::intersectPlanes(const FacePtr &a, const FacePtr &b, const FacePtr &c) {
    fvec3 n0 = a->m_planeNormal;
    fvec3 n1 = b->m_planeNormal;
    fvec3 n2 = c->m_planeNormal;

    float denom = dot(cross(n0, n1), n2);
    if (denom < CMP_EPSILON)
      return boolRet<Vertex>(false, fv3zero);

    Vertex v = {};
    v.point =
        (cross(n1, n2) * a->m_planeDist + cross(n2, n0) * b->m_planeDist + cross(n0, n1) * c->m_planeDist) /
        denom;

    return boolRet<Vertex>(true, v);
  }

  Vertex Brush::mergeDuplicate(int from, Vertex &v) {
    for (int n = 0; n <= from; n++) {
      auto otherPoly = m_faces[n];
      for (int i = 0; i < otherPoly->m_vertices.size(); i++) {
        if (dist3(otherPoly->m_vertices[i].point, v.point) < CMP_EPSILON) {
          return otherPoly->m_vertices[i];
        }
      }
    }
    return v;
  }

  void Brush::indexFaceVertices() {

    for (auto &f : m_faces) {
      if (f->m_vertices.size() < 3)
        continue;

      f->m_indices.clear();
      f->m_indices.reserve((f->m_vertices.size() - 2) * 3);
      for (int i = 0; i < f->m_vertices.size() - 2; i++) {
        f->m_indices.push_back(0);
        f->m_indices.push_back(i + 1);
        f->m_indices.push_back(i + 2);
      }
      f->UpdateNormals();
    }
  }

  void Brush::windFaceVertices() {

    for (auto &f : m_faces) {
      if (f->m_vertices.size() < 3)
        continue;

      auto windFaceBasis = normalize(f->m_vertices[1].point - f->m_vertices[0].point);
      auto windFaceCenter = fvec3();
      auto windFaceNormal = normalize(f->m_planeNormal);

      for (auto v : f->m_vertices) {
        windFaceCenter += v.point;
      }
      windFaceCenter /= (float)f->m_vertices.size();

      std::stable_sort(f->m_vertices.begin(), f->m_vertices.end(), [&](Vertex l, Vertex r) {
        fvec3 u = normalize(windFaceBasis);
        fvec3 v = normalize(cross(u, windFaceNormal));

        fvec3 loc_a = l.point - windFaceCenter;
        float a_pu = dot(loc_a, u);
        float a_pv = dot(loc_a, v);

        fvec3 loc_b = r.point - windFaceCenter;
        float b_pu = dot(loc_b, u);
        float b_pv = dot(loc_b, v);

        float a_angle = atan2(a_pv, a_pu);
        float b_angle = atan2(b_pv, b_pu);

        if (a_angle == b_angle) {
          return 0;
        }
        return a_angle > b_angle ? 0 : 1;
      });
    }
  }

  static Vertex interpolate(const Vertex &v1, const Vertex &v2, float t) {
    Vertex res;
    res.point = v1.point + (v2.point - v1.point) * t;
    res.uv = v1.uv + (v2.uv - v1.uv) * t;
    res.normal = v1.normal;
    res.tangent = v1.tangent;
    res.lightmap_uv = v1.lightmap_uv + (v2.lightmap_uv - v1.lightmap_uv) * t;
    return res;
  }

  void Brush::splitFace(const FacePtr &in, const FacePtr &plane, FacePtr &front, FacePtr &back) {
    if (!in || in->m_vertices.empty())
      return;

    std::vector<Vertex> fVerts, bVerts;
    const auto &verts = in->m_vertices;
    const auto &normal = plane->m_planeNormal;
    const float dist = plane->m_planeDist;

    std::vector<float> dists;
    dists.reserve(verts.size());

    double split_epsilon = CMP_EPSILON;

    for (const auto &v : verts) {
      dists.push_back(dot(normal, v.point) - dist);
    }

    for (size_t i = 0; i < verts.size(); ++i) {
      const auto &v1 = verts[i];
      float d1 = dists[i];

      size_t next = (i + 1) % verts.size();
      const auto &v2 = verts[next];
      float d2 = dists[next];

      if (d1 >= -split_epsilon)
        fVerts.push_back(v1);
      if (d1 <= split_epsilon)
        bVerts.push_back(v1);

      if ((d1 > split_epsilon && d2 < -split_epsilon) || (d1 < -split_epsilon && d2 > split_epsilon)) {
        float t = d1 / (d1 - d2);
        Vertex mid = interpolate(v1, v2, t);
        fVerts.push_back(mid);
        bVerts.push_back(mid);
      }
    }

    if (fVerts.size() >= 3) {
      front = in->Copy();
      front->m_vertices = fVerts;
      front->m_indices.clear();
    } else
      front = nullptr;

    if (bVerts.size() >= 3) {
      back = in->Copy();
      back->m_vertices = bVerts;
      back->m_indices.clear();
    } else
      back = nullptr;
  }

  void Brush::clipFace(FacePtr face, FaceIter plane, const FaceIter &planeEnd, std::vector<FacePtr> &outFaces,
                       bool keepOnPlane, bool isCoplanar) {
    if (face->Type() == MapSurface::CLIP || face->Type() == MapSurface::SKIP)
      return;

    if (plane == planeEnd) {
      if (isCoplanar && keepOnPlane) {
        outFaces.push_back(face);
      }
      return;
    }

    if ((*plane)->Type() != MapSurface::SOLID) {
      clipFace(face, plane + 1, planeEnd, outFaces, keepOnPlane, isCoplanar);
      return;
    }

    auto ecp = (*plane)->Classify(face.get());
    switch (ecp) {
    case MapSurface::FRONT:
      outFaces.push_back(face);
      return;
    case MapSurface::BACK:
      clipFace(face, plane + 1, planeEnd, outFaces, keepOnPlane, isCoplanar);
      return;
    case MapSurface::ON_PLANE: {
      double angle = dot(face->m_planeNormal, (*plane)->m_planeNormal) - 1;
      if ((angle < CMP_EPSILON) && (angle > -CMP_EPSILON)) {
        clipFace(face, plane + 1, planeEnd, outFaces, keepOnPlane, true);
        return;
      }
      clipFace(face, plane + 1, planeEnd, outFaces, keepOnPlane, isCoplanar);
      return;
    }
    case MapSurface::SPANNING: {
      FacePtr front = nullptr, back = nullptr;
      splitFace(face, *plane, front, back);
      if (front)
        outFaces.push_back(front);
      if (back)
        clipFace(back, plane + 1, planeEnd, outFaces, keepOnPlane, isCoplanar);
      return;
    }
    }
  }

  std::vector<FacePtr> Brush::clipToBrush(const Brush &other, bool keepOnPlane) {
    std::vector<FacePtr> clippedFaces;
    auto planes_begin = other.m_faces.begin();
    auto planes_end = other.m_faces.cend();

    for (auto &face : m_faces) {
      clipFace(face, planes_begin, planes_end, clippedFaces, keepOnPlane);
    }
    return clippedFaces;
  }

  fvec3 GetUnitNormal(const fvec2 p1, const fvec2 p2, const float s) {
    const fvec2 p3 = p1 + ((p2 - p1) * s);

    const float m = (p3[1] - p1[1]) / (p3[0] - p1[0]);
    const float c = p1[1] - m * p1[0];
    const float y = (m * p1[0]) + c;

    const fvec2 tangent = normalize(fvec2(p1[0], p2[1]));
    fvec3 normal = fvec3(-tangent[1], 0, tangent[0]);

    return normalize(normal);
  }

  void Brush::generatePolygons(const std::map<int, MapSurface::eFaceType> &faceTypes,
                               const std::map<int, textureBounds> &texBounds) {
    float phongAngle = 89.0;
    for (int i = 0; i < m_faces.size(); i++) {
      if (m_faces[i] == nullptr)
        continue;

      for (int j = 0; j < m_faces.size(); j++)
        for (int k = 0; k < m_faces.size(); k++) {
          if (i == j && i == k && j == k)
            continue;

          if (m_faces[i] == nullptr || m_faces[j] == nullptr || m_faces[k] == nullptr) {
            continue;
          }
          auto kv = faceTypes.find(m_faces[k]->m_textureID);
          if (kv != faceTypes.end()) {
            m_faces[k]->m_type = kv->second;
            if (m_faces[k]->m_type == MapSurface::CLIP) {
              m_isBlockVolume = true;
            }
          }

          auto res = intersectPlanes(m_faces[i], m_faces[j], m_faces[k]);
          if (!res.first || !isLegalVertex(res.second, m_faces)) {
            continue;
          }

          res.second = mergeDuplicate(i, res.second);

          auto v = res.second;
          v.normal = m_faces[i]->m_planeNormal;
          v.normal = normalize(v.normal);
          v.tangent = m_faces[k]->CalcTangent();

          auto tb = texBounds.find(m_faces[k]->m_textureID);
          if (tb != texBounds.end() && (tb->second.width > 0 && tb->second.height > 0)) {
            v.uv = m_faces[k]->CalcUV(v.point, tb->second.width, tb->second.height);
          }

          v.lightmap_uv = m_faces[k]->CalcLightmapUV(v.point);

          if (v.inList(m_faces[k]->m_vertices))
            continue;
          m_faces[k]->m_vertices.push_back(v);
        }

      m_faces[i]->UpdateAB();
    }
  }

  bool Brush::isLegalVertex(const Vertex &v, const std::vector<FacePtr> &faces) {
    for (const auto &f : faces) {
      auto proj = tue::math::dot(f->m_planeNormal, v.point);
      if (proj > f->m_planeDist && fabs(f->m_planeDist - proj) > 0.0008) {
        return false;
      }
    }
    return true;
  }

  bool Brush::DoesIntersect(const Brush &other) {
    if ((min[0] > other.max[0]) || (other.min[0] > max[0]))
      return false;

    if ((min[1] > other.max[1]) || (other.min[1] > max[1]))
      return false;

    if ((min[2] > other.max[2]) || (other.min[2] > max[2]))
      return false;

    return true;
  }

  void Brush::calculateAABB() {
    if (m_faces[0]->m_vertices.size() == 0)
      return;

    min = m_faces[0]->m_vertices[0].point;
    max = m_faces[0]->m_vertices[0].point;

    for (const auto &face : m_faces)
      for (const auto &vert : face->m_vertices) {
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
}
