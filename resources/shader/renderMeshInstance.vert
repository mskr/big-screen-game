#version 330 core

#define M_PI 3.1415926535897932384626433832795
#define M_HALF_PI 1.5707963267948966192313216916397
#define M_THREE_OVER_TWO_PI 4.7123889803846898576939650749192

#define EMPTY 0
#define INSIDE_ROOM 1
#define CORNER 2
#define WALL 4
#define TOP 8
#define BOTTOM 16
#define RIGHT 32
#define LEFT 64
#define INVALID 128
#define SOURCE 256
#define INFECTED 512
#define OUTER_INFLUENCE 1024

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
uniform float automatonTimeDelta;

uniform float t_sec;

out vec3 vPosition;
out vec3 vNormal;
out vec2 vTexCoords;

vec2 rotateZ_step90(int st, float x, float y) {
	// Problem: One mesh can be used with different instance attributes for different build states
	// Very specific solution: Choose rotation for corners and walls
	//TODO Find more generic solution
	if((buildState & (LEFT | TOP | CORNER))==(LEFT|TOP|CORNER) ||
		(buildState & (LEFT | WALL))==(LEFT | WALL)){
			return vec2(y, -x);
	}
	else if((buildState & (RIGHT | TOP | CORNER))==(RIGHT|TOP|CORNER) ||
		(buildState & (TOP | WALL))==(TOP | WALL)){
			return vec2(-x, -y);
	}
	else if((buildState & (RIGHT | BOTTOM | CORNER))==(RIGHT|BOTTOM|CORNER) ||
		(buildState & (RIGHT | WALL))==(RIGHT | WALL)){
			return vec2(-y, x);
	}else{
			return vec2(x, y);
	}



	//switch(buildState) {
	//	case BSTATE_LEFT_UPPER_CORNER:
	//	case BSTATE_WALL_LEFT:
	//		return vec2(y, -x);
	//	case BSTATE_RIGHT_UPPER_CORNER:
	//	case BSTATE_WALL_TOP:
	//		return vec2(-x, -y);
	//	case BSTATE_RIGHT_LOWER_CORNER:
	//	case BSTATE_WALL_RIGHT:
	//		return vec2(-y, x);
	//	default:
	//		return vec2(x, y);
	//}
}

flat out int st;
flat out int hp;
out vec2 cellCoords;

void main() {
	mat4 modelMatrix = mat4(0); // this fixed the glitch
	modelMatrix[3] = vec4(translation, 1);
	modelMatrix[0][0] = scale;
	modelMatrix[1][1] = scale;
	modelMatrix[2][2] = scale;

	st = buildState;
	hp = health;
	cellCoords = translation.xy + vec2(1, 1 + gridCellSize) - gridTranslation.xy;
	cellCoords += (texCoords.yx - 0.5) * gridCellSize;
	cellCoords /= gridDimensions;

	if((buildState & OUTER_INFLUENCE)!=0) {
		const float WATER_WAVE_LENGTH = 20.0;
		const float WATER_WAVE_HEIGHT = 40.0;
		float WATER_WAVE_DIRECTION = cellCoords.x;
		//modelMatrix[3][2] += ((1.0 + sin(t_sec * WATER_WAVE_DIRECTION * WATER_WAVE_LENGTH)) / WATER_WAVE_HEIGHT);
	}

	vec4 posV4 = modelMatrix * subMeshLocalMatrix * vec4(rotateZ_step90(buildState, position.x, -position.z), position.y, 1);
	vPosition = vec3(posV4);
	vNormal = vec3(rotateZ_step90(buildState, normal.x, -normal.z), normal.y); //TODO incorporate sin wave
	vTexCoords = texCoords;

	gl_Position = viewProjectionMatrix * posV4;
}