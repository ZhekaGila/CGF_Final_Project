#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aColor;
// These attributes advance once per spectator, not once per vertex.
layout(location = 4) in vec4 aPositionScale;
layout(location = 5) in vec4 aTintPhase;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform bool uAudience;
uniform float uAudienceTime;
uniform float uAudienceEnergy;

out vec3 vColor;
out vec3 vNormal;
out vec3 vWorldPos;
out vec2 vUV;

void main()
{
    vec4 worldPosition;
    if (uAudience) {
        float phase = aTintPhase.w;
        float beat = uAudienceTime * (3.8 + 0.25 * sin(phase)) + phase;
        float bounce = max(sin(beat), 0.0) * 0.24 * uAudienceEnergy;
        float sway = sin(uAudienceTime * 1.8 + phase) * 0.065 * uAudienceEnergy;
        float c = cos(sway), s = sin(sway);
        mat3 rotation = mat3(c, s, 0.0, -s, c, 0.0, 0.0, 0.0, 1.0);
        vec3 position = rotation * aPosition * aPositionScale.w;
        worldPosition = vec4(position + aPositionScale.xyz + vec3(0.0, bounce, 0.0), 1.0);
        vNormal = normalize(rotation * aNormal); // scale is uniform and positive
        // Only the white shirt/arms receive the per-instance clothing tint.
        float clothing = step(0.99, min(aColor.r, min(aColor.g, aColor.b)));
        vColor = aColor * mix(vec3(1.0), aTintPhase.rgb, clothing);
    } else {
        worldPosition = uModel * vec4(aPosition, 1.0);
        vNormal = normalize(mat3(transpose(inverse(uModel))) * aNormal);
        vColor = aColor;
    }
    vWorldPos = worldPosition.xyz;
    vUV = aUV;
    gl_Position = uProjection * uView * worldPosition;
}
