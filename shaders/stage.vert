#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec3 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vColor;
out vec3 vNormal;

void main(){
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    
    gl_Position = uProjection * uView * worldPosition;

    vColor = aColor;
    
    mat3 normalMatrix = mat3(transpose(inverse(uModel)));
    vNormal = normalize(normalMatrix * aNormal);
}