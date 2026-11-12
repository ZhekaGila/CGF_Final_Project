#version 330 core

in vec3 vColor;
in vec3 vNormal;

out vec4 FragColor;

void main()
{
    vec3 normal = normalize(vNormal);

    vec3 lightDirection = normalize(vec3(-0.4, 0.8, 0.5));

    float diffuse = max(dot(normal, lightDirection), 0.0);

    float ambient = 0.35;

    vec3 lighting = vColor * (ambient + 0.65 * diffuse);

    FragColor = vec4(lighting, 1.0);
}
