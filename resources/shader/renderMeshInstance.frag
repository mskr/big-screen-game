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

struct Material {
    float alpha;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
//    float shininess;
};

uniform Material material;

uniform int isDepthPass;
uniform int isDebugMode;

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
	
	float healthNormalized = float(hp)/100;

	// if current mesh instance is INFECTED, apply some effect
	if((st & INFECTED) != 0) {
		color = vec4(1,0,0,.5);
	}
	else { // else visualize normals
//		color = vec4(vNormal, 1) * healthNormalized;
        vec3 col = material.ambient+material.diffuse+material.specular;
        color = vec4(col,1) * healthNormalized;

	}
}