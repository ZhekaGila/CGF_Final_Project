#version 330 core

layout(location = 0) in vec2 aPos;

uniform mat4 uMVP;
uniform float uRadius;
uniform float uTime;
uniform float uPhase;

out float vAngle;

void main()
{
    float angle = atan(aPos.y, aPos.x);
    vAngle = angle;

    float wave1 = sin(angle * 4.0 + uTime * 0.9 + uPhase) * 0.045;
    float wave2 = sin(angle * 6.0 - uTime * 0.6 + uPhase) * 0.018;

    float waveRadius = uRadius * (1.0 + wave1 + wave2);

    vec2 position = aPos * waveRadius;

    gl_Position = uMVP * vec4(position, 0.0, 1.0);
}
