#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;

layout(location = 3) in vec3 translation;
layout(location = 4) in vec3 scale;
layout(location = 5) in float zRotation;

uniform mat4 subMeshLocalMatrix;
uniform mat3 normalMatrix;
uniform mat4 viewProjectionMatrix;

out vec3 vPosition;
out vec3 vNormal;
out vec2 vTexCoords;

void main()
{
	mat4 modelMatrix;
	modelMatrix[3] = vec4(translation, 1);
	modelMatrix[0][0] = scale.x;
	modelMatrix[1][1] = scale.y;
	modelMatrix[2][2] = scale.z;
    vec4 posV4 = subMeshLocalMatrix * modelMatrix * vec4(position, 1);
    vPosition = vec3(posV4);
    vNormal = vec3(normalize(modelMatrix * vec4(normal,1)));
    vTexCoords = texCoords;

    gl_Position = viewProjectionMatrix * posV4;
}

