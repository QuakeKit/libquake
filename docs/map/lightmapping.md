# Lightmapping System

Lightmapping pre-calculates lighting for map surfaces and stores it in a texture atlas. This creates high-quality static lighting with minimal runtime cost. This document explains how `LightmapGenerator` works using code examples.

## 1. Generation Pipeline

The generation process has four stages:

1.  **Surface Parameterization** - Calculate UV coordinates for each face
2.  **Chart Packing** - Arrange all face lightmaps into an atlas texture
3.  **Coordinate Remapping** - Update UVs to reference the final atlas
4.  **Radiance Evaluation** - Calculate lighting for each pixel (luxel)

### Stage 1: Surface Parameterization (Planar Mapping)

Each face needs 2D coordinates for its lightmap. Quake uses **Planar Projection** based on the texture axes from the .MAP file.

**Calculating Lightmap UVs:**

```cpp
// For each vertex on a face
vec2 CalculateLightmapUV(vec3 vertexPos, Face face) {
    // Project 3D position onto 2D plane using texture axes
    float u = dot(vertexPos, face.uAxis);
    float v = dot(vertexPos, face.vAxis);
    
    return vec2(u, v);
}
```

**Finding Face Bounds:**

```cpp
struct FaceBounds {
    float minU, maxU;
    float minV, maxV;
};

FaceBounds CalculateFaceBounds(Face face) {
    FaceBounds bounds = {INFINITY, -INFINITY, INFINITY, -INFINITY};
    
    // Check all vertices to find min/max
    for (Vertex vertex : face.vertices) {
        vec2 uv = CalculateLightmapUV(vertex.position, face);
        
        bounds.minU = min(bounds.minU, uv.x);
        bounds.maxU = max(bounds.maxU, uv.x);
        bounds.minV = min(bounds.minV, uv.y);
        bounds.maxV = max(bounds.maxV, uv.y);
    }
    
    return bounds;
}
```

**Calculating Lightmap Size:**

```cpp
// How many lightmap pixels (luxels) does this face need?
struct LightmapSize {
    int width;
    int height;
};

LightmapSize CalculateLightmapSize(FaceBounds bounds) {
    const float LUXEL_SIZE = 16.0f;  // One lightmap pixel per 16 world units
    
    float extentU = bounds.maxU - bounds.minU;
    float extentV = bounds.maxV - bounds.minV;
    
    // Round up and add 1 for padding
    int width = (int)ceil(extentU / LUXEL_SIZE) + 1;
    int height = (int)ceil(extentV / LUXEL_SIZE) + 1;
    
    return {width, height};
}
```

### Stage 2: Chart Packing (Atlas Generation)

After calculating dimensions for every face, we arrange them into a single texture atlas without overlapping.

**Algorithm: First-Fit Decreasing Height**

This "shelf packing" strategy efficiently packs rectangles:

```cpp
struct PackedFace {
    int width, height;      // Lightmap dimensions
    int atlasX, atlasY;     // Position in atlas
};

void LightmapGenerator::PackFaces(std::vector<PackedFace>& faces) {
    const int ATLAS_SIZE = 2048;  // 2048x2048 atlas texture
    
    // 1. Sort faces by height (tallest first)
    //    This ensures large items establish row heights efficiently
    std::sort(faces.begin(), faces.end(), 
        [](const PackedFace& a, const PackedFace& b) {
            return a.height > b.height;
        });
    
    // 2. Pack faces row by row
    int currentX = 0;      // Cursor position
    int currentY = 0;
    int rowHeight = 0;     // Height of current row
    
    for (PackedFace& face : faces) {
        // Does this face fit in the current row?
        if (currentX + face.width > ATLAS_SIZE) {
            // No - start a new row
            currentY += rowHeight;
            currentX = 0;
            rowHeight = 0;
        }
        
        // Place face at current position
        face.atlasX = currentX;
        face.atlasY = currentY;
        
        // Advance cursor
        currentX += face.width;
        rowHeight = max(rowHeight, face.height);
    }
}
```

**Why This Works:**
- Sorts by height (O(n log n)) to minimize wasted vertical space
- Greedy row packing fits items left-to-right
- Simple and fast for typical face size distributions


### Stage 3: Coordinate Remapping

After packing, vertex UVs are relative to individual faces (0..1 range). We need to remap them to the final atlas coordinates.

```cpp
// Remap a vertex's local lightmap UV to atlas UV
vec2 RemapToAtlas(vec2 localUV, PackedFace face, FaceBounds bounds) {
    const int ATLAS_SIZE = 2048;
    const float LUXEL_SIZE = 16.0f;
    
    // 1. Convert local UV (0..1) back to world units
    float worldU = bounds.minU + (localUV.x * (bounds.maxU - bounds.minU));
    float worldV = bounds.minV + (localUV.y * (bounds.maxV - bounds.minV));
    
    // 2. Convert to lightmap pixel coordinates
    float luxelU = (worldU - bounds.minU) / LUXEL_SIZE;
    float luxelV = (worldV - bounds.minV) / LUXEL_SIZE;
    
    // 3. Offset by atlas position
    float atlasPixelX = luxelU + face.atlasX;
    float atlasPixelY = luxelV + face.atlasY;
    
    // 4. Normalize to 0..1 range for the entire atlas
    vec2 atlasUV;
    atlasUV.x = atlasPixelX / ATLAS_SIZE;
    atlasUV.y = atlasPixelY / ATLAS_SIZE;
    
    return atlasUV;
}

// Apply to all vertices
void RemapFaceVertices(Face& face, PackedFace& packed, FaceBounds bounds) {
    for (Vertex& vertex : face.vertices) {
        vertex.lightmapUV = RemapToAtlas(vertex.lightmapUV, packed, bounds);
    }
}
```

Now each vertex correctly references its portion of the atlas texture.


### Stage 4: Radiance Evaluation (Baking)

The final step calculates the color of every lightmap pixel (luxel) by simulating light from all light entities.

**Reconstructing World Position:**

To light a luxel, we need its 3D position. We reverse the planar projection:

```cpp
// Convert luxel coordinates back to world space
vec3 LuxelToWorld(int luxelX, int luxelY, PackedFace face, FaceBounds bounds) {
    const float LUXEL_SIZE = 16.0f;
    
    // 1. Convert luxel pixel to UV coordinates
    float u = bounds.minU + (luxelX * LUXEL_SIZE);
    float v = bounds.minV + (luxelY * LUXEL_SIZE);
    
    // 2. Reconstruct 3D position using texture axes
    vec3 worldPos = face.origin;
    worldPos += u * face.uAxis;
    worldPos += v * face.vAxis;
    
    // 3. Offset slightly along normal to prevent shadow acne
    worldPos += face.normal * 0.5f;
    
    return worldPos;
}
```

**Lighting Model: Lambertian with Distance Falloff**

For each luxel, we calculate light from all light entities:

```cpp
struct Light {
    vec3 position;
    vec3 color;
    float radius;
};

vec3 CalculateLuxelColor(vec3 luxelPos, vec3 normal, std::vector<Light>& lights) {
    // Start with ambient lighting
    vec3 finalColor = vec3(0.1f, 0.1f, 0.1f);  // Dark ambient
    
    // Add contribution from each light
    for (const Light& light : lights) {
        // Vector from luxel to light
        vec3 lightDir = light.position - luxelPos;
        float distance = length(lightDir);
        
        // Skip if light is too far away
        if (distance > light.radius) {
            continue;
        }
        
        // Normalize direction
        lightDir = normalize(lightDir);
        
        // Calculate attenuation (light falloff with distance)
        // Creates smooth sphere of influence
        float attenuation = 1.0f - (distance / light.radius);
        attenuation = attenuation * attenuation;  // Squared for smooth falloff
        attenuation = max(0.0f, attenuation);
        
        // Calculate diffuse lighting (Lambertian reflection)
        // Surfaces facing the light are brighter
        float diffuse = dot(normal, lightDir);
        diffuse = max(0.0f, diffuse);  // Clamp to 0 for backfacing
        
        // Add this light's contribution
        vec3 contribution = light.color * diffuse * attenuation;
        finalColor += contribution;
    }
    
    return finalColor;
}
```

**Baking the Atlas:**

```cpp
void BakeLightmap(PackedFace face, std::vector<Light>& lights, 
                  uint8_t* atlasData, int atlasWidth) {
    // Iterate every pixel in this face's atlas region
    for (int y = 0; y < face.height; y++) {
        for (int x = 0; x < face.width; x++) {
            // Get world position for this luxel
            vec3 worldPos = LuxelToWorld(x, y, face, face.bounds);
            
            // Calculate lighting
            vec3 color = CalculateLuxelColor(worldPos, face.normal, lights);
            
            // Convert to RGB bytes and write to atlas
            int atlasX = face.atlasX + x;
            int atlasY = face.atlasY + y;
            int index = (atlasY * atlasWidth + atlasX) * 3;
            
            atlasData[index + 0] = (uint8_t)clamp(color.r * 255, 0, 255);
            atlasData[index + 1] = (uint8_t)clamp(color.g * 255, 0, 255);
            atlasData[index + 2] = (uint8_t)clamp(color.b * 255, 0, 255);
        }
    }
}
```

**Important Notes:**
- This implementation does **not** cast shadow rays - geometry doesn't occlude light
- Lighting is purely based on distance and surface angle
- For shadows, you'd need ray tracing or BSP traversal (not currently implemented)

---

## References

1.  **Quake Engine Source Code**: `r_light.c` (Original surface lighting implementation).
2.  **Texture Packing**:
    *   *Rectangle Packing on Wikipedia*: [Bin packing problem#2D_packing](https://en.wikipedia.org/wiki/Bin_packing_problem)
    *   *Shelf Packing Algorithm*: Commonly used for sprite sheets and font atlases.
3.  **Lighting Models**:
    *   *Lambert's Cosine Law*: [Wikipedia](https://en.wikipedia.org/wiki/Lambert%27s_cosine_law)
    *   *Attenuation Models*: [Valve Developer Community: Constant-Linear-Quadratic Falloff](https://developer.valvesoftware.com/wiki/Light#Falloff)
