#version 330 core
#define M_PI 3.1415926535897932384626433832795

// Vertex attribs
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;

// Instance attribs
layout(location = 3) in vec3 translation;
layout(location = 4) in vec3 scale;
layout(location = 5) in float zRotation;
layout(location = 6) in int health;

uniform mat4 subMeshLocalMatrix;
uniform mat3 normalMatrix;
uniform mat4 viewProjectionMatrix;

out vec3 vPosition;
out vec3 vNormal;
out vec2 vTexCoords;
flat out int vHealth;

mat4 rotationMatrix(vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;
    
    return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

void main()
{
	mat4 modelMatrix;
	modelMatrix[3] = vec4(translation, 1);
	modelMatrix[0][0] = scale.x;
	modelMatrix[1][1] = scale.y;
	modelMatrix[2][2] = scale.z;
	mat4 rotation = rotationMatrix(vec3(0,0,1), zRotation);
	// Flip everything 90 deg around X (could also change models)
	rotation *= rotationMatrix(vec3(1,0,0), -M_PI/2.0);
	modelMatrix *= rotation;

    vec4 posV4 = modelMatrix * subMeshLocalMatrix * vec4(position, 1);
    vPosition = vec3(posV4);
    vNormal = normalize(mat3(rotation) * normal);
    vTexCoords = texCoords;
    vHealth = health;

    gl_Position = viewProjectionMatrix * posV4;
}

