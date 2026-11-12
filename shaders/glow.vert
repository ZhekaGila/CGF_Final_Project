#version 330 core

layout(location = 0) in vec2 aPosition;

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uGlowPosition;
uniform float uGlowSize;

out vec2 vUV;

void main()
{
    vUV = aPosition * 0.5 + 0.5;

    vec3 cameraRight = vec3(
        uView[0][0],
        uView[1][0],
        uView[2][0]
    );

    vec3 cameraUp = vec3(
        uView[0][1],
        uView[1][1],
        uView[2][1]
    );

    vec3 worldPosition =
        uGlowPosition +
        cameraRight * aPosition.x * uGlowSize +
        cameraUp * aPosition.y * uGlowSize;

    gl_Position = uProjection * uView *
        vec4(worldPosition, 1.0);
}
