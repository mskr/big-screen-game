#version 330 core

#define M_PI 3.1415926535897932384626433832795
#define M_HALF_PI 1.5707963267948966192313216916397
#define M_THREE_OVER_TWO_PI 4.7123889803846898576939650749192

#define BSTATE_EMPTY 0
#define BSTATE_INSIDE_ROOM 1
#define BSTATE_LEFT_UPPER_CORNER 2
#define BSTATE_RIGHT_UPPER_CORNER 3
#define BSTATE_LEFT_LOWER_CORNER 4
#define BSTATE_RIGHT_LOWER_CORNER 5
#define BSTATE_WALL_LEFT 6
#define BSTATE_WALL_RIGHT 7
#define BSTATE_WALL_TOP 8
#define BSTATE_WALL_BOTTOM 9
#define BSTATE_INVALID 10
#define BSTATE_OUTER_INFLUENCE 11

// Vertex attribs
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoords;

// Instance attribs
layout(location = 3) in vec3 translation;
layout(location = 4) in float scale;
layout(location = 5) in int buildState;
layout(location = 6) in int health;

uniform mat4 subMeshLocalMatrix;
uniform mat3 normalMatrix;
uniform mat4 viewProjectionMatrix;

uniform vec2 gridDimensions;
uniform vec3 gridTranslation;
uniform float gridCellSize;

out vec3 vPosition;
out vec3 vNormal;
out vec2 vTexCoords;

mat4 rotationMatrix(vec3 axis, float angle) {
	axis = normalize(axis);
	float s = sin(angle);
	float c = cos(angle);
	float oc = 1.0 - c;
	return mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
				oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
				oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
				0.0,                                0.0,                                0.0,                                1.0);
}

float chooseZRotation(const int st) {
	// Problem: One mesh can be used with different instance attributes for different build states
	// Very specific solution: Choose rotation for corners and walls
	//TODO Find more generic solution
	if (st == BSTATE_LEFT_UPPER_CORNER || st == BSTATE_WALL_LEFT || st == BSTATE_OUTER_INFLUENCE)
		return M_HALF_PI;
	else if (st == BSTATE_RIGHT_UPPER_CORNER || st == BSTATE_WALL_TOP)
		return M_PI;
	else if (st == BSTATE_RIGHT_LOWER_CORNER || st == BSTATE_WALL_RIGHT)
		return M_THREE_OVER_TWO_PI;
	else
		return 0.0;
}

flat out int st;
flat out int hp;
out vec2 cellCoords;

void main() {
	mat4 modelMatrix;
	modelMatrix[3] = vec4(translation, 1);
	modelMatrix[0][0] = scale;
	modelMatrix[1][1] = scale;
	modelMatrix[2][2] = scale;
	mat4 rotation = rotationMatrix(vec3(0,0,1), chooseZRotation(buildState));
	// Flip everything 90 deg around X (could also change models)
	rotation *= rotationMatrix(vec3(1,0,0), -M_HALF_PI);
	modelMatrix *= rotation;

	st = buildState;
	hp = health;
	cellCoords = (translation.xy + 1.0 - gridTranslation.xy) / gridDimensions;
	cellCoords += vec2(texCoords.x -.5, 1 - texCoords.y +.5) * gridCellSize;

	vec4 posV4 = modelMatrix * subMeshLocalMatrix * vec4(position, 1);
	vPosition = vec3(posV4);
	vNormal = normalize(mat3(rotation) * normal);
	vTexCoords = texCoords;

	gl_Position = viewProjectionMatrix * posV4;
}

