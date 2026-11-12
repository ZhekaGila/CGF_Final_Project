#version 330 core

in vec2 vUV;

out vec4 FragColor;

uniform vec3 uGlowColor;

void main()
{
    float distanceFromCenter = length(
        vUV - vec2(0.5)
    ) * 2.0;

    float glow = 1.0 - smoothstep(
        0.0,
        1.0,
        distanceFromCenter
    );

    glow = pow(glow, 1.2);

    float alpha = glow * 0.45;

    FragColor = vec4(uGlowColor, alpha);
}
