#version 330 core

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

uniform int isDepthPass;
uniform int isDebugMode;

// threshold to discard "low-value" outer influence pixels
const float OUTER_INFLUENCE_DISPLAY_THRESHOLD = 0.6;

out vec4 color;

void main() {
	if(isDepthPass == 1) return;
	if(isDebugMode == 1) {
		color = vec4(1);
		return;
	}

	//TODO lighting
	// vec3 lightDir = normalize(vPosition - vec3(-10.0f, -10.0f, -10.0f));

	// float NdotL = clamp(dot(lightDir, normalize(vNormal)), 0.0f, 1.0f);
	// vec3 texColor = texture(diffuseTexture, vTexCoords).rgb;

	//TODO Why are the edges still not completely black?
	//TODO What to do with the time delta? Need the texture with the previous state as well?
	
	float healthNormalized = float(hp)/100;
	if((st & OUTER_INFLUENCE)!=0) {
		float v_prev = texture(gridTex_PrevState, cellCoords).r;
		float v = texture(gridTex, cellCoords).r;
		v = mix(v_prev, v, automatonTimeDelta);
		v *= (255.0 / OUTER_INFLUENCE);
		if(v < OUTER_INFLUENCE_DISPLAY_THRESHOLD)
			discard;
		if(v > 0.65) // "high-value" threshold
			color = vec4(0, v-0.1, v, 0.5) * healthNormalized;
		else // edge between high-value and discarded pixels
			color = vec4(1) * healthNormalized;
	}
	else {
		color = vec4(vNormal, 1) * healthNormalized;
	}
}