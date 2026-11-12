#version 330 core

in vec3 vColor;
in vec3 vNormal;
in vec3 vWorldPos;

out vec4 FragColor;

uniform vec3 uCameraPos;

void main()
{
    vec3 normal = normalize(vNormal);

    vec3 lightDir = normalize(vec3(-0.4, 0.8, 0.5));

    float ambientStrength = 0.25;
    vec3 ambient = ambientStrength *vColor;

    float diffuseStrength = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diffuseStrength * 0.65 * vColor;

    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float specularStrength = 0.35;
    float shininess = 32.0;

    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * vec3(1.0);

    vec3 result = ambient + diffuse + specular;

    FragColor = vec4(result, 1.0);
}
