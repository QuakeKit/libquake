# Constructive Solid Geometry (CSG)

To render the map efficiently and correctly, we must determine which surfaces are visible and remove hidden geometry (e.g., the inside of a wall).

## Approach: Additive CSG with Clipping
We treat the world as a void filled with solid brushes. Since brushes are convex, they are simple to process individually. Complex shapes are formed by multiple overlapping brushes.

## The Algorithm
1. **Brush-vs-Brush Interaction**: We iterate through every pair of brushes in an entity.
2. **Intersection Test**: AABB (Axis-Aligned Bounding Box) checks quickly eliminate non-interacting brushes.
3. **Clipping**: If Brush A intersects Brush B:
    - Brush A's faces are split by the infinite planes of Brush B.
    - Fragments inside Brush B are discarded (hidden).
    - Fragments outside are retained.
4. **Cleanup**:
    - **T-Junction Fixing**: Splitting faces creates new vertices on edges, causing gaps. We stitch these T-junctions.
    - **Vertex Welding**: Co-located vertices are merged to ensure watertight meshes.
    - **Collinear Removal**: Redundant vertices on straight lines are removed.

## Geometry Post-Processing
After the CSG clipping phase, the geometry is often fragmented. To produce high-quality meshes for game engines, we apply three critical clean-up passes.

### 1. Vertex Welding
**Problem:** CSG operations generate many duplicate vertices at identical locations (within floating-point error). This increases memory usage and breaks mesh connectivity (topology).

**Solution:** We collect all vertices and snap those within a small `epsilon` (0.005 units) to a single shared position.

```cpp
void SolidMapEntity::weldVertices() {
    double weld_epsilon = 0.005;

    // 1. Collect all vertices pointers
    std::vector<Vertex *> allVerts = collectAllVertices();

    // 2. Sort by X-coordinate for fast spatial lookup
    std::sort(allVerts.begin(), allVerts.end(), ...);

    // 3. Iterate and snap neighbors
    for (size_t i = 0; i < allVerts.size(); ++i) {
        for (size_t j = i + 1; j < allVerts.size(); ++j) {
            // Optimization: Stop if X-distance exceeds epsilon
            if (allVerts[j]->point.x - allVerts[i]->point.x > weld_epsilon)
                break;

            // Snap if distance is within epsilon
            if (dist3(allVerts[i]->point, allVerts[j]->point) < weld_epsilon) {
                allVerts[j]->point = allVerts[i]->point;
            }
        }
    }
}
```

### 2. T-Junction Fixing
**Problem:** A "T-Junction" occurs when a vertex from one face lies on the edge of a neighboring face but isn't part of that neighbor's vertex list. During rendering, floating-point rounding errors can cause a visible pixel gap ("sparkle") along this edge.

**Solution:** We find these "mid-edge" vertices and insert them into the edge of the affected face, effectively splitting the edge.

**Algorithm:**
1.  Collect all unique vertex positions in the map.
2.  For every edge `(V1, V2)` of every face:
    *   Find all global vertices `P` that lie on the line segment `V1-V2`.
    *   If `P` is close to the line but not equal to `V1` or `V2`, it's a T-junction.
    *   Insert `P` into the face's vertex list between `V1` and `V2`.

```cpp
void SolidMapEntity::fixTJunctions() {
    auto uniqueVerts = getUniqueVertices();

    for (auto &face : allFaces) {
        for (edge : face.edges) {
             // Find vertices that lie ON this edge
             std::vector<fvec3> splits = findVerticesOnEdge(edge, uniqueVerts);
             
             // Insert them into the face
             if (!splits.empty()) {
                 face.insertVertices(edge, splits);
             }
        }
    }
}
```

### 3. Collinear Removal
**Problem:** The previous steps (clipping and T-junction fixing) can leave many redundant vertices along a straight line. For example, a square face might end up with 5 vertices on one side. This complicates triangulation and wastes GPU resources.

**Solution:** We iterate through face loops and remove the middle vertex of any three consecutive vertices that form a straight line (180-degree angle/zero cross product).

```cpp
void SolidMapEntity::removeCollinearVertices() {
    for (auto &face : allFaces) {
        bool changed = true;
        while (changed) {
            changed = false;
            auto &verts = face.vertices;
            
            for (size_t i = 0; i < verts.size(); ++i) {
                // Get Previous, Current, Next vertices
                vec3 prev = verts[(i - 1) % n];
                vec3 curr = verts[i];
                vec3 next = verts[(i + 1) % n];

                vec3 e1 = normalize(curr - prev);
                vec3 e2 = normalize(next - curr);

                // If cross product is ~0, vectors are parallel (collinear)
                if (length(cross(e1, e2)) < EPSILON) {
                    verts.erase(i); // Remove redundant vertex 'curr'
                    changed = true;
                    break;
                }
            }
        }
    }
}
```

## Reference
- [CSG on Wikipedia](https://en.wikipedia.org/wiki/Constructive_solid_geometry)
- [Brush Clipping (Fabien Sanglard)](https://fabiensanglard.net/doom3/dmap.php)
