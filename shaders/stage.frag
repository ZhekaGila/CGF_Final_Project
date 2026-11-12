#version 330 core

#define NUM_LIGHTS 4

in vec3 vColor;
in vec3 vNormal;
in vec3 vWorldPos;

out vec4 FragColor;

uniform vec3 uCameraPos;

struct SpotLight
{
    vec3 position;
    vec3 direction;
    vec3 color;
    float cutOff;
    float outerCutOff;
};

uniform SpotLight uLights[NUM_LIGHTS];

vec3 calculateSpotLight(
    SpotLight light,
    vec3 normal,
    vec3 viewDir
)
{
    vec3 toLight = light.position - vWorldPos;
    float distance = length(toLight);
    vec3 lightDir = normalize(toLight);

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * vColor * light.color;

    vec3 halfwayDir = normalize(lightDir + viewDir);

    float spec = 0.0;

    if (diff > 0.0)
    {
        spec = pow(
            max(dot(normal, halfwayDir), 0.0),
            32.0
        );
    }

    vec3 specular = 0.35 * spec * light.color;

    float theta = dot(
        lightDir,
        normalize(-light.direction)
    );

    float epsilon = max(
        light.cutOff - light.outerCutOff,
        0.0001
    );

    float intensity = clamp(
        (theta - light.outerCutOff) / epsilon,
        0.0,
        1.0
    );

    float attenuation = 1.0 /
        (1.0 + 0.045 * distance + 0.0075 * distance * distance);

    return (diffuse + specular) * intensity * attenuation;
}

void main()
{
    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(uCameraPos - vWorldPos);

    vec3 result = 0.25 * vColor;

    for (int i = 0; i < NUM_LIGHTS; i++)
    {
        result += calculateSpotLight(
            uLights[i],
            normal,
            viewDir
        );
    }

    FragColor = vec4(result, 1.0);
}
