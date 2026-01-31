#include <quakelib/map/entities.h>

namespace quakelib::map {

  static Vertex interpolate(const Vertex &v1, const Vertex &v2, float t) {
    Vertex res;
    res.point = v1.point + (v2.point - v1.point) * t;
    res.uv = v1.uv + (v2.uv - v1.uv) * t;
    res.normal = v1.normal;
    res.tangent = v1.tangent;
    res.lightmap_uv = v1.lightmap_uv + (v2.lightmap_uv - v1.lightmap_uv) * t;
    return res;
  }

  void SolidMapEntity::csgUnion() {
    if (!m_brushes.empty()) {
      m_min = m_brushes[0].min;
      m_max = m_brushes[0].max;
    }
    for (auto &b1 : m_brushes) {
      // Skip CSG for non-solid brushes (CLIP/SKIP/NODRAW) - export them as-is
      if (b1.IsNonSolidBrush()) {
        m_clippedBrushes.push_back(b1);
        b1.GetBiggerBBox(m_min, m_max);
        continue;
      }

      auto cpBrush = b1;
      for (auto &b2 : m_brushes) {
        if (&b1 == &b2 || b2.m_faces.empty()) {
          continue;
        }

        // Don't clip against non-solid brushes
        if (b2.IsNonSolidBrush()) {
          continue;
        }

        if (!b1.DoesIntersect(b2) || (b1.IsBlockVolume() || b2.IsBlockVolume())) {
          continue;
        }

        bool keepOnPlane = &b1 < &b2;
        auto clippedFaces = cpBrush.clipToBrush(b2, keepOnPlane);
        cpBrush.m_faces = clippedFaces;
      }
      if (!cpBrush.m_faces.empty()) {
        cpBrush.indexFaceVertices();
        m_clippedBrushes.push_back(cpBrush);
        cpBrush.GetBiggerBBox(m_min, m_max);
        m_stats_clippedFaces +=
            static_cast<long long>(b1.m_faces.size()) - static_cast<long long>(cpBrush.m_faces.size());
      }
    }
    m_center = math::CalculateCenterFromBBox(m_min, m_max);
    if (m_brushes.size() > 0 && m_clippedBrushes.size() > 0) {
      m_wasClipped = true;
      weldVertices();
      fixTJunctions();
      removeCollinearVertices();
      triangulateFaces();
    }
  }

  void SolidMapEntity::weldVertices() {
    std::vector<Vertex *> allVerts;
    double weld_epsilon = 0.005;

    auto &targetBrushes = m_clippedBrushes.empty() ? m_brushes : m_clippedBrushes;

    for (auto &b : targetBrushes) {
      for (auto &f : b.Faces()) {
        for (auto &v : f->m_vertices) {
          allVerts.push_back(&v);
        }
      }
    }

    std::sort(allVerts.begin(), allVerts.end(),
              [](const Vertex *a, const Vertex *b) { return a->point[0] < b->point[0]; });

    for (size_t i = 0; i < allVerts.size(); ++i) {
      for (size_t j = i + 1; j < allVerts.size(); ++j) {
        float dx = allVerts[j]->point[0] - allVerts[i]->point[0];
        if (dx > weld_epsilon)
          break;

        if (dist3(allVerts[i]->point, allVerts[j]->point) < weld_epsilon) {
          allVerts[j]->point = allVerts[i]->point;
        }
      }
    }
  }

  void SolidMapEntity::fixTJunctions() {
    auto &targetBrushes = m_clippedBrushes.empty() ? m_brushes : m_clippedBrushes;
    double edge_epsilon = 0.05;

    std::vector<Vec3> uniqueVerts;
    uniqueVerts.reserve(targetBrushes.size() * 32);

    for (const auto &b : targetBrushes) {
      for (const auto &f : b.Faces()) {
        for (const auto &v : f->m_vertices) {
          uniqueVerts.push_back(v.point);
        }
      }
    }

    std::sort(uniqueVerts.begin(), uniqueVerts.end(), [](const Vec3 &a, const Vec3 &b) {
      if (std::abs(a[0] - b[0]) > 0.001)
        return a[0] < b[0];
      if (std::abs(a[1] - b[1]) > 0.001)
        return a[1] < b[1];
      return a[2] < b[2];
    });

    auto last = std::unique(uniqueVerts.begin(), uniqueVerts.end(),
                            [](const Vec3 &a, const Vec3 &b) { return dist3(a, b) < 0.001; });
    uniqueVerts.erase(last, uniqueVerts.end());

    for (auto &b : targetBrushes) {
      bool modified = false;
      for (auto &f : b.Faces()) {
        std::vector<Vertex> newVerts;
        const auto &oldVerts = f->m_vertices;
        size_t count = oldVerts.size();

        if (count < 3)
          continue;

        for (size_t i = 0; i < count; ++i) {
          const auto &v1 = oldVerts[i];
          const auto &v2 = oldVerts[(i + 1) % count];

          newVerts.push_back(v1);

          Vec3 dir = v2.point - v1.point;
          float len = math::Len(dir);
          if (len < edge_epsilon)
            continue;

          Vec3 dirNorm = math::Norm(dir);
          std::vector<Vec3> splits;

          float min_x = std::min(v1.point[0], v2.point[0]) - edge_epsilon;
          float max_x = std::max(v1.point[0], v2.point[0]) + edge_epsilon;

          auto it = std::lower_bound(uniqueVerts.begin(), uniqueVerts.end(), min_x,
                                     [](const Vec3 &p, float val) { return p[0] < val; });

          for (; it != uniqueVerts.end() && (*it)[0] <= max_x; ++it) {
            const auto &testP = *it;

            if (dist3(testP, v1.point) < edge_epsilon || dist3(testP, v2.point) < edge_epsilon)
              continue;

            Vec3 v1_to_p = testP - v1.point;
            float t = math::Dot(v1_to_p, dirNorm);

            if (t > edge_epsilon && t < len - edge_epsilon) {
              Vec3 closest = v1.point + (dirNorm * t);
              if (dist3(closest, testP) < edge_epsilon) {
                splits.push_back(testP);
              }
            }
          }

          if (splits.empty())
            continue;

          std::sort(splits.begin(), splits.end(),
                    [&](const Vec3 &a, const Vec3 &b) { return dist3(a, v1.point) < dist3(b, v1.point); });

          for (const auto &splitP : splits) {
            if (!newVerts.empty() && dist3(newVerts.back().point, splitP) < 0.001)
              continue;

            float t = math::Len(splitP - v1.point) / len;
            Vertex splitV = interpolate(v1, v2, t);
            splitV.point = splitP;
            newVerts.push_back(splitV);
            modified = true;
          }
        }
        f->m_vertices = newVerts;
      }
      if (modified) {
        b.indexFaceVertices();
      }
    }
  }

  void SolidMapEntity::removeCollinearVertices() {
    auto &targetBrushes = m_wasClipped ? m_clippedBrushes : m_brushes;

    for (auto &brush : targetBrushes) {
      for (const auto &face : brush.m_faces) {
        auto &verts = face->m_vertices;
        if (verts.size() < 3)
          continue;

        bool changed = true;
        while (changed && verts.size() >= 3) {
          changed = false;
          for (size_t i = 0; i < verts.size(); ++i) {
            size_t prev = (i + verts.size() - 1) % verts.size();
            size_t next = (i + 1) % verts.size();

            Vec3 p = verts[prev].point;
            Vec3 c = verts[i].point;
            Vec3 n = verts[next].point;

            Vec3 e1 = c - p;
            Vec3 e2 = n - c;

            if (math::Len(e1) < math::CMP_EPSILON || math::Len(e2) < math::CMP_EPSILON) {
              verts.erase(verts.begin() + i);
              changed = true;
              break;
            }

            if (math::Len(math::Cross(math::Norm(e1), math::Norm(e2))) < math::CMP_EPSILON) {
              verts.erase(verts.begin() + i);
              changed = true;
              break;
            }
          }
        }
      }
      brush.indexFaceVertices();
    }
  }

  void SolidMapEntity::triangulateFaces() {
    auto isPointInTriangle = [](const Vec2 &p, const Vec2 &a, const Vec2 &b, const Vec2 &c) {
      auto sign = [](const Vec2 &p1, const Vec2 &p2, const Vec2 &p3) {
        return (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1]);
      };
      float d1 = sign(p, a, b);
      float d2 = sign(p, b, c);
      float d3 = sign(p, c, a);
      bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
      bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
      return !(has_neg && has_pos);
    };

    auto &targetBrushes = m_wasClipped ? m_clippedBrushes : m_brushes;

    for (auto &brush : targetBrushes) {
      std::vector<FacePtr> newFaces;
      newFaces.reserve(brush.m_faces.size() * 2);

      for (auto &face : brush.m_faces) {
        if (face->m_vertices.size() <= 3) {
          newFaces.push_back(face);
          continue;
        }

        std::vector<Vertex> &verts = face->m_vertices;
        Vec3 normal = face->GetPlaneNormal();

        int axis = 0;
        float nx = std::abs(normal[0]);
        float ny = std::abs(normal[1]);
        float nz = std::abs(normal[2]);

        if (ny > nx && ny > nz)
          axis = 1;
        else if (nz > nx && nz > ny)
          axis = 2;

        auto project = [&](const Vec3 &v) -> Vec2 {
          if (axis == 0)
            return {v[1], v[2]};
          if (axis == 1)
            return {v[0], v[2]};
          return {v[0], v[1]};
        };

        std::vector<int> indices(verts.size());
        for (size_t i = 0; i < verts.size(); ++i)
          indices[i] = i;

        int count = static_cast<int>(indices.size());
        int limit = count * 2;

        while (count > 2) {
          if (limit-- <= 0)
            break;

          bool earFound = false;
          for (int i = 0; i < count; ++i) {
            int idxPrev = (i + count - 1) % count;
            int idxCurr = i;
            int idxNext = (i + 1) % count;

            int nPrev = indices[idxPrev];
            int nCurr = indices[idxCurr];
            int nNext = indices[idxNext];

            const Vertex &vP = verts[nPrev];
            const Vertex &vC = verts[nCurr];
            const Vertex &vN = verts[nNext];

            Vec3 edgeA = vC.point - vP.point;
            Vec3 edgeB = vN.point - vC.point;
            Vec3 crossP = math::Cross(edgeA, edgeB);

            if (math::Dot(crossP, normal) <= -1e-4f)
              continue;

            bool containsPoint = false;
            Vec2 p2 = project(vP.point);
            Vec2 c2 = project(vC.point);
            Vec2 n2 = project(vN.point);

            for (int k = 0; k < count; ++k) {
              if (k == idxPrev || k == idxCurr || k == idxNext)
                continue;

              if (isPointInTriangle(project(verts[indices[k]].point), p2, c2, n2)) {
                containsPoint = true;
                break;
              }
            }

            if (!containsPoint) {
              FacePtr tri = face->Copy();
              tri->m_vertices.clear();
              tri->m_vertices.reserve(3);
              tri->m_vertices.push_back(vP);
              tri->m_vertices.push_back(vC);
              tri->m_vertices.push_back(vN);
              tri->m_indices = {0, 1, 2};

              tri->UpdateNormals();
              newFaces.push_back(tri);

              indices.erase(indices.begin() + idxCurr);
              count--;
              earFound = true;
              break;
            }
          }
          if (!earFound) {
            for (size_t i = 1; i < indices.size() - 1; ++i) {
              FacePtr tri = face->Copy();
              tri->m_vertices.clear();
              tri->m_vertices.reserve(3);
              tri->m_vertices.push_back(verts[indices[0]]);
              tri->m_vertices.push_back(verts[indices[i]]);
              tri->m_vertices.push_back(verts[indices[i + 1]]);
              tri->m_indices = {0, 1, 2};
              tri->UpdateNormals();
              newFaces.push_back(tri);
            }
            break;
          }
        }
      }
      brush.m_faces = newFaces;
    }
  }

} // namespace quakelib::map
