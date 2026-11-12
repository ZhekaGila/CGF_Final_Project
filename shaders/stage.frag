#version 330 core

in vec3 vColor;
in vec3 vNormal;
in vec3 vWorldPos;

out vec4 FragColor;

uniform vec3 uCameraPos;

uniform vec3 uLightPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;

uniform float uCutOff;
uniform float uOuterCutOff;

void main()
{
    vec3 normal = normalize(vNormal);

    vec3 lightDir = normalize(uLightPos - vWorldPos);

    vec3 viewDir = normalize(uCameraPos - vWorldPos);

    vec3 ambient = 0.20 * vColor;

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 diffuse = diff * vColor * uLightColor;

    vec3 halfwayDir = normalize(lightDir + viewDir);

    float spec = 0.0;

    if(diff > 0.0)
    {
        spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    }

    vec3 specular = 0.35 * spec * uLightColor;
    
    float theta = dot(lightDir, normalize(-uLightDir));

    float epsilon = max(uCutOff - uOuterCutOff, 0.0001);

    float intensity = clamp((theta - uOuterCutOff) / epsilon, 0.0, 1.0);

    float distance = length(uLightPos - vWorldPos);

    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));

    vec3 lighting = ambient + (diffuse + specular) * intensity * attenuation;

    FragColor = vec4(lighting, 1.0);
}
