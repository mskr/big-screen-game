#version 330 core

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

// uniform sampler2D diffuseTexture;

in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoords;

flat in int st;
flat in int hp;

in vec2 cellCoords;
uniform sampler2D gridTex;
uniform sampler2D gridTex_PrevState;
uniform float automatonTimeDelta;

// threshold to discard "low-value" outer influence pixels
const float OUTER_INFLUENCE_DISPLAY_THRESHOLD = 0.5;

out vec4 color;

void main()
{
	// vec3 lightDir = normalize(vPosition - vec3(-10.0f, -10.0f, -10.0f));

	// float NdotL = clamp(dot(lightDir, normalize(vNormal)), 0.0f, 1.0f);
	// vec3 texColor = texture(diffuseTexture, vTexCoords).rgb;

	//TODO Why are the edges still not completely black?
	//TODO What to do with the time delta? Need the texture with the previous state as well?
	
	float healthNormalized = float(hp)/100;
	if(st==BSTATE_OUTER_INFLUENCE) {
		float v_prev = texture(gridTex_PrevState, cellCoords).r;
		float v = texture(gridTex, cellCoords).r;
		v = mix(v_prev, v, automatonTimeDelta);
		v *= (255.0 / BSTATE_OUTER_INFLUENCE);
		if(v < OUTER_INFLUENCE_DISPLAY_THRESHOLD)
			discard;
		if(v > 0.6) v = 1;
		color = vec4(v, 0, 0, 1) * healthNormalized;
	}
	else {
		color = vec4(vNormal, 1) * healthNormalized;
	}
}