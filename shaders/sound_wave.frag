#version 330 core

out vec4 FragColor;

uniform float uAlpha;
uniform float uColorOffset;

void main()
{
    vec3 colors[5] = vec3[5](
        vec3(0.10, 0.75, 1.00), // Cyan
        vec3(0.20, 0.95, 0.55), // Green
        vec3(1.00, 0.85, 0.25), // Yellow
        vec3(1.00, 0.35, 0.55), // Pink
        vec3(0.55, 0.30, 1.00)  // Purple
    );

    
    float t = mod(uColorOffset, 5.0);

    int first = int(floor(t));
    int second = (first + 1) % 5;

    float blend = fract(t);
    blend = blend * blend * (3.0 - 2.0 * blend);

    vec3 color = mix(colors[first], colors[second], blend);

    FragColor = vec4(color, uAlpha);
}
