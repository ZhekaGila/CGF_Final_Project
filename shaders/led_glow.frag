#version 330 core

in vec2 vPosition;

out vec4 FragColor;

uniform vec3 uColor;
uniform float uIntensity;

void main()
{
    float distanceY = abs(vPosition.y);

    float innerGlow = exp(-distanceY * distanceY * 18.0);
    float outerGlow = exp(-distanceY * distanceY * 4.0);

    float edgeFade =
        1.0 - smoothstep(0.94, 1.0, abs(vPosition.x));

    float verticalFade =
        1.0 - smoothstep(0.75, 1.0, distanceY);

    float alpha =
        (innerGlow * 0.45 + outerGlow * 0.25)
        * edgeFade
        * verticalFade
        * uIntensity;

    FragColor = vec4(uColor, alpha);
}