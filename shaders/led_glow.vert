#version 330 core

layout(location = 0) in vec2 aPos;

uniform mat4 uView;
uniform mat4 uProjection;

uniform vec3 uCenter;
uniform vec3 uAxis;
uniform float uLength;
uniform float uHeight;

out vec2 vPosition;

void main()
{
    vPosition = aPos;

    vec3 worldPos =
        uCenter
        + uAxis * aPos.x * uLength * 0.5
        + vec3(0.0, aPos.y * uHeight * 0.5, 0.0);

    gl_Position =
        uProjection * uView * vec4(worldPos, 1.0);
}