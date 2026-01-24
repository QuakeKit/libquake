# Lightmapping System

Lightmapping in Quake allows for high-quality static lighting with low runtime cost. It pre-calculates the incident light at surfaces and stores the result in a texture (the Lightmap Atlas). This document details the specific algorithms used in `LightmapGenerator`.

## 1. Generation Pipeline

The generation process occurs in four distinct stages:

1.  **Surface Parameterization (UV Generation)**
2.  **Chart Packing (Atlas Generation)**
3.  **Coordinate Remapping**
4.  **Radiance Evaluation (Baking)**

### Stage 1: Surface Parameterization (Planar Mapping)

For every solid face in the map, we generate a set of 2D coordinates. Unlike modern UV unwrapping which might use Least Squares Conformal Maps (LSCM), Quake uses **Planar Projection** based on the texture axes defined in the .MAP file actions.

Given a vertex position $P(x,y,z)$, the lightmap UVs $(u, v)$ are calculated via dot products with the surface's texture vectors:

$$
u = (P \cdot \vec{U}_{axis})
$$
$$
v = (P \cdot \vec{V}_{axis})
$$

Where $\vec{U}_{axis}$ and $\vec{V}_{axis}$ are derived from the plane normal to ensure minimal distortion.

*   **Bounds Calculation**: We iterate all vertices of a face to find $min(u,v)$ and $max(u,v)$.
*   **Dimensions**: The integer dimensions of the lightmap chart for a face are:
    $$
    W = \lceil \frac{max_u - min_u}{\text{LuxelSize}} \rceil + 1
    $$
    $$
    H = \lceil \frac{max_v - min_v}{\text{LuxelSize}} \rceil + 1
    $$
    *(Note: LuxelSize is typically 16 world units).*

### Stage 2: Chart Packing (Atlas Generation)

Once we have the dimensions ($W \times H$) for every face, we must arrange them into a single large texture (The Atlas) without overlapping.

**Algorithm: First-Fit Decreasing Height (Row Packing)**
The implementation uses a "shelf" or "row-based" packing strategy optimized for rectangular items.

1.  **Sort**: All faces are sorted by **height in descending order**. This essentially "solves" the Knapsack Problem more efficiently for rectangular packing by ensuring large items establish the row height.
2.  **Row Filling**:
    *   We maintain a `CurrentX`, `CurrentY`, and `RowHeight`.
    *   If the current face fits in the remaining width of the current row:
        *   Place it at `(CurrentX, CurrentY)`.
        *   Advance `CurrentX` by `FaceWidth`.
    *   If it does *not* fit:
        *   Move to the next row: `CurrentY += RowHeight`.
        *   Reset `CurrentX = 0`.
        *   Reset `RowHeight = FaceHeight` (since this is the first, tallest item of the new row).

```cpp
void LightmapGenerator::Pack() {
    // 1. Sort faces by height
    std::sort(faces.begin(), faces.end(), 
        [](const auto& a, const auto& b) { return a.h > b.h; });

    int currentX = 0;
    int currentY = 0;
    int rowH = 0;

    for (auto& face : faces) {
        // Check if we need a new row
        if (currentX + face.w > ATLAS_SIZE) {
            currentY += rowH;
            currentX = 0;
            rowH = 0;
        }

        // Store packing coordinates
        face.atlasX = currentX;
        face.atlasY = currentY;

        // Advance cursor
        currentX += face.w;
        rowH = std::max(rowH, face.h);
    }
}
```

### Stage 3: Coordinate Remapping

After packing, the lightmap UVs stored on the vertices are still relative to the individual face (0..1 range for that specific face chart). They must be remapped to global texture coordinates (0..1 range for the entire atlas).

$$
u_{final} = \frac{face\_min_u + (u_{local} \times face\_w) + atlas\_x}{ATLAS\_SIZE}
$$

This ensures the geometry correctly samples its allocated portion of the giant lightmap texture.
    *   For each face:
        *   If `CurrentX + FaceW > AtlasWidth`: Move to next row (`CurrentY += RowHeight`, `CurrentX = 0`, `RowHeight = 0`).
        *   Place face at `(CurrentX, CurrentY)`.
        *   Update `RowHeight = max(RowHeight, FaceH)`.
        *   `CurrentX += FaceW`.

This simple greedy algorithm is $O(n \log n)$ due to sorting and provides sufficiently dense packing for lightmaps where rectangles are somewhat uniform in size distribution.

---

## 3. Coordinate Remapping

After determining the position $(AtlasX, AtlasY)$ for a face, we must update the mesh vertices to point to this new location.

$$
UV_{atlas}.x = \frac{\frac{(UV_{local}.x - UV_{min}.x)}{\text{LuxelSize}} + AtlasX}{\text{AtlasWidth}}
$$
$$
UV_{atlas}.y = \frac{\frac{(UV_{local}.y - UV_{min}.y)}{\text{LuxelSize}} + AtlasY}{\text{AtlasHeight}}
$$

This maps the $[0, 1]$ range of the lightmap UV channel to the specific sub-rectangle in the generated atlas.

---

## 4. Radiance Evaluation (Baking)

The final step is to calculate the color of every pixel (luxel) in the allocated charts.

### Luxel-to-World Reconstruction
We iterate over every pixel $(x, y)$ in a face's allocated block. To light it, we need its World Position. We invert the planar mapping:

$$
P_{world} = P_{origin} + (u \cdot \vec{U}_{axis}) + (v \cdot \vec{V}_{axis}) + (0.5 \cdot \vec{N})
$$

*(We shift the position slightly along the normal $N$ to avoid self-intersection artifacts known as "shadow acne").*

### Lighting Model: Lambertian with Quadratic Falloff
For each luxel at world position $P_{luxel}$ with normal $N$:

1.  **Correction**: Initialize with `AmbientColor`.
2.  **Accumulation**: For each Light $L$ in the scene:
    *   **Vector**: $\vec{V}_{light} = L_{pos} - P_{luxel}$
    *   **Distance**: $d = |\vec{V}_{light}|$
    *   **Culling**: If $d > L_{radius}$, skip.
    *   **Attenuation**: We use a modified quadratic falloff that creates a smooth sphere of influence:
        $$
        Atten = \max(0, 1 - \frac{d}{L_{radius}})^2
        $$
    *   **Lambertian Term**: The cosine law of reflection:
        $$
        Diffuse = \max(0, \vec{N} \cdot \text{normalize}(\vec{V}_{light}))
        $$
    *   **Result**: $Color += L_{color} \cdot Diffuse \cdot Atten$

**Note**: The current implementation does **not** cast shadow rays (Ray-trace or BSP traversal); geometry does not occlude light. It is purely distance and normal based.

---

## References

1.  **Quake Engine Source Code**: `r_light.c` (Original surface lighting implementation).
2.  **Texture Packing**:
    *   *Rectangle Packing on Wikipedia*: [Bin packing problem#2D_packing](https://en.wikipedia.org/wiki/Bin_packing_problem)
    *   *Shelf Packing Algorithm*: Commonly used for sprite sheets and font atlases.
3.  **Lighting Models**:
    *   *Lambert's Cosine Law*: [Wikipedia](https://en.wikipedia.org/wiki/Lambert%27s_cosine_law)
    *   *Attenuation Models*: [Valve Developer Community: Constant-Linear-Quadratic Falloff](https://developer.valvesoftware.com/wiki/Light#Falloff)
