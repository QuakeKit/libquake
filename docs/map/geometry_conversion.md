# Map Construction

The process of converting raw `.map` data into renderable geometry involves three stages: Parsing, Brush Construction, and Meshing.

## 1. Parsing
The `.map` file is text-based. The parser reads:
- **Entities**: Container objects (Worldspawn, lights, doors).
- **Brushes**: Convex volumes defined by sets of planes.
- **Planes**: Defined by 3 points.

## 2. Brush Construction
Brushes are defined mathematically as the intersection of half-spaces. A "solid" brush exists in the region that is "behind" all of its defining planes.

### Algorithm: From Planes to Polygons
To convert this mathematical definition into a renderable mesh (vertices and faces), we perform a 3-plane intersection test for every possible combination of planes in the brush.

**The Logic:**
1.  **Iterate Triplets**: Select every possible combination of 3 planes $(P_i, P_j, P_k)$ from the brush.
2.  **Calculate Intersection**: Find the single point $V$ where these 3 planes meet.
3.  **Validate**: Check if $V$ lies "inside" (or on) every other plane in the brush.
    *   If $V$ is in front of *any* plane, it is outside the brush volume and is discarded.
    *   If $V$ is behind or on all planes, it is a valid vertex of the brush.
4.  **Assign**: Add valid vertex $V$ to the faces defined by $P_i$, $P_j$, and $P_k$.

### Code Example
```cpp
void Brush::generatePolygons() {
    // 3 loops to select every unique triplet of faces (planes)
    for (int i = 0; i < faces.size(); i++)
      for (int j = i + 1; j < faces.size(); j++)
        for (int k = j + 1; k < faces.size(); k++) {

          // 1. Find the intersection point of the 3 planes
          //    Uses Cramer's rule / cross products
          Vertex v = intersectPlanes(faces[i], faces[j], faces[k]);

          // 2. Check if this point is legally inside the brush volume
          if (!isLegalVertex(v, faces)) {
            continue; // Outside the brush -> discard
          }

          // 3. Add to the 3 contributing faces
          faces[i]->addVertex(v);
          faces[j]->addVertex(v);
          faces[k]->addVertex(v);
        }
}
```

### Vertex Validation
The `isLegalVertex` check is the most critical part. It ensures convexity.

```cpp
bool Brush::isLegalVertex(const Vertex &v, const std::vector<Face> &faces) {
    for (const auto &plane : faces) {
        // Calculate distance from point to plane
        float dist = dot(plane.normal, v.point) - plane.dist;
        
        // If distance > epsilon, point is in front of plane -> Illegal
        if (dist > EPSILON) {
            return false; 
        }
    }
    return true;
}
```

## 3. Winding
Once vertices are assigned to faces, they are unordered clouds of points. To render a polygon, vertices must be ordered (usually Counter-Clockwise).

**Algorithm:**
1.  Calculate a "Center" point (average of all vertices).
2.  Project vectors from Center to each Vertex.
3.  Sort vertices by angle around the face's normal vector.

## Reference
- [Quake Map Format](https://quakewiki.org/wiki/Quake_Map_Format)
- [Convex Polyhedra](https://en.wikipedia.org/wiki/Convex_polytope)
