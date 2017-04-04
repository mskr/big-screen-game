#version 330 core
#define M_PI 3.1415926535897932384626433832795

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
layout(location = 4) in vec3 scale;
layout(location = 5) in float zRotation;
layout(location = 6) in int buildState0;
layout(location = 7) in int buildState1;
layout(location = 8) in ivec2 neighborBuildStatesPacked0; // x=N,NE,E,SE y=S,SW,W,NW
layout(location = 9) in ivec2 neighborBuildStatesPacked1;
layout(location = 10) in int health;

uniform float automatonTimeDelta;

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

ivec4 unpack(int p) {
    ivec4 a;
    for(int i = 0; i < 4; i++) {
        a[i] = (p << ((3-i)*8)) >> (3*8);
    }
    return a;
}

int unpack(int p, int i) {
	return (p << ((3-i)*8)) >> (3*8);
}

#define N 0
#define NE 1
#define E 2
#define SE 3
#define S 4
#define SW 5
#define W 6
#define NW 7
int unpack(ivec2 p, int i) {
	if(i<4) return (p.x << ((3-i)*8)) >> (3*8);
	else return (p.y << ((7-i)*8)) >> (3*8);
}

bool neighbor(int i) {
	return unpack(neighborBuildStatesPacked1,i)==BSTATE_OUTER_INFLUENCE;
}

flat out int buildState;

void main()
{
	vec3 interpolated_pos = position;
	/*
	if(buildState0==BSTATE_EMPTY && buildState1==BSTATE_OUTER_INFLUENCE) {
		vec4 from = vec4(1,0,1,0);
		vec4 to = vec4(1,0,1,0);
		//TODO Animate the birth of a outer influence cell based on:
		// a) Pre-transition and post-transition neighbor build states
		// b) Vertex position (x,z=0,0 is middle of cell)
		interpolated_pos.xz = mix(from.xz*position.xz+from.yw, to.xz*position.xz+to.yw, automatonTimeDelta);
	}
	*/
	//TODO Animate remaining cell too (based on neighbor changes)
	//TODO How to get information to animate the death?

	buildState = buildState1;
    vHealth = health;

	/*if(buildState==BSTATE_OUTER_INFLUENCE) {
		float w;
		if(position.x > 0.5) {
			if(position.z > 0.5) { // NE
				// w = 0.6;
				c.r = 1;
			}
			else if(position.z < -0.5) { // SE
				// w = 0.6;
				c.g = 1;
			}
			else { // E
				// w = 1.0;
				c.b = 1;
			}
		}
		else if(position.x < -0.5) {
			if(position.z > 0.5) { // NW
				// w = 0.6;
				c.r = 0.5;
			}
			else if(position.z < -0.5) { // SW
				// w = 1.0;
				// c.g = 0.5;
			}
			else { // W
				// w = 1.0;
				// c.b = 0.5;
			}
		}
		else {
			if(position.z > 0.5) { // N
				// w = 1.0;
				// c.r = 0.1;
			}
			else if(position.z < -0.5) { // S
				// w = 1.0;
				// c.g = 0.1;
			}
		}
		interpolated_pos.xz = w*position.xz;
	}*/

	mat4 modelMatrix;
	modelMatrix[3] = vec4(translation, 1);
	modelMatrix[0][0] = scale.x;
	modelMatrix[1][1] = scale.y;
	modelMatrix[2][2] = scale.z;
	mat4 rotation = rotationMatrix(vec3(0,0,1), zRotation);
	// Flip everything 90 deg around X (could also change models)
	rotation *= rotationMatrix(vec3(1,0,0), -M_PI/2.0);
	modelMatrix *= rotation;

    vec4 posV4 = modelMatrix * subMeshLocalMatrix * vec4(interpolated_pos, 1);
    vPosition = vec3(posV4);
    vNormal = normalize(mat3(rotation) * normal);
    vTexCoords = texCoords;

    gl_Position = viewProjectionMatrix * posV4;
}

