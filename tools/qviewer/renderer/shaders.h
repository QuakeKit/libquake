#pragma once

const char *VS_CODE = R"(
#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec2 vertexTexCoord2; // Lightmap UV
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;
out vec2 fragTexCoord2;
out vec4 fragColor;
out vec3 fragNormal;
out vec3 fragPosition;

void main()
{
    // Send vertex attributes to fragment shader
    fragTexCoord = vertexTexCoord;
    fragTexCoord2 = vertexTexCoord2;
    fragColor = vertexColor;
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));

    // Calculate final vertex position
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

const char *FS_CODE = R"(
#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec2 fragTexCoord2;
in vec4 fragColor;
in vec3 fragNormal;
in vec3 fragPosition;

// Input uniform values
uniform sampler2D texture0; // Diffuse
uniform sampler2D texture1; // Lightmap (assigned to METALNESS slot)
uniform vec4 colDiffuse;
uniform float renderWireframe; // 1.0 = show wireframe, 0.0 = hide
uniform vec3 viewPos; // Camera position for distance fading

// Output fragment color
out vec4 finalColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec4 lightmapColor = texture(texture1, fragTexCoord2);
    
    // Wireframe calculation from barycentric coordinates (stored in vertexColor)
    // We look for the closest edge (smallest component)
    float closestEdge = min(fragColor.r, min(fragColor.g, fragColor.b));
    float edgeWidth = 0.02;

    vec4 baseColor = texelColor * colDiffuse;

    if (renderWireframe > 0.5) {
        float closestEdge = min(fragColor.r, min(fragColor.g, fragColor.b));
        float edgeWidth = 0.005;

        if (closestEdge < edgeWidth) {
            float dist = distance(fragPosition, viewPos);
            float fadeStart = 5.0;
            float fadeEnd = 40.0;
            float alpha = 1.0 - clamp((dist - fadeStart) / (fadeEnd - fadeStart), 0.0, 1.0);
            
            finalColor = vec4(0.0, 1.0, 0.0, alpha); // Solid Green Wireframe with fade
        } else {
            // Discard pixel to allow seeing through (X-Ray)
            // Or render transparently if blending is enabled, but discard is safer for generic depthless rendering
            discard; 
        }
    } else {
        // Multiply lightmap with diffuse
        vec4 baseVal = (texture(texture0, fragTexCoord) * colDiffuse) * lightmapColor;

        // Fog
        float dist = distance(fragPosition, viewPos);
        float fogDensity = 0.005;
        // Exponential Fog: 1.0 - exp(-distance * density)
        float fogFactor = 1.0 - exp(-dist * fogDensity);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        vec4 fogColor = vec4(0.25, 0.1, 0.35, 1.0); // Purple

        finalColor = mix(baseVal, fogColor, fogFactor);
    }
}
)";

const char *FS_SKY_CODE = R"(
#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec2 fragTexCoord2;
in vec4 fragColor;
in vec3 fragNormal;
in vec3 fragPosition;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float time; // for scrolling
uniform vec3 viewPos;

// Output fragment color
out vec4 finalColor;

void main()
{
    // Quake-style Sphere Mapping (Skybox effect)
    vec3 dir = normalize(fragPosition - viewPos);
    
    // Convert direction to spherical UVs
    // p = (atan(z, x) / 2pi, y) roughly
    // Quake's sky texture is typically 256x128. 
    // It maps horizontally 360 degrees.
    // We adjust scale for the look:
    
    float u = (atan(dir.z, dir.x) / (2.0 * 3.14159)) * 4.0; // Repeat 4 times?
    float v = (asin(dir.y) / (3.14159 / 2.0)) + 0.5; // Map -1..1 to 0..1? No, Quake sky is distinct.
    
    // Better approximation for "Projected Planar" top:
    // Quake actually treats the sky as two infinite planes mostly? No it's a box?
    // Let's try simple spherical first.
    
    vec2 skyUV = vec2(u, dir.y); 

    // Simple scrolling sky
    // Layer 1
    vec2 scroll1 = vec2(time * 0.05, time * 0.01);
    vec4 tex1 = texture(texture0, skyUV + scroll1);
    
    // Layer 2 (simulated by sampling same texture at different offset/scale)
    vec2 scroll2 = vec2(time * 0.02, time * 0.03);
    vec4 tex2 = texture(texture0, (skyUV * 2.0) + scroll2);
    
    // Blend layers
    vec4 skyColor = mix(tex1, tex2, 0.5);
    
    finalColor = skyColor * colDiffuse;
}
)";
