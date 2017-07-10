#version 330 core


// Speed of water shader
#define speed 2

// the amount of shearing (shifting of a single column or row)
// 1.0 = entire screen height offset (to both sides, meaning it's 2.0 in total)
#define xDistMag 0.05
#define yDistMag 0.05

// cycle multiplier for a given screen height
// 2*PI = you see a complete sine wave from top..bottom
#define xSineCycles 6.28
#define ySineCycles 6.28

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;
uniform float time;

uniform int isDebugMode;

in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoords;
in vec4 vPosLightSpace;
in vec2 fragCoords;

out vec4 color;

const float DEPTH_BIAS = 0.00001;

float visibility(vec3 thisFragment) {
	float v = 0.0;
	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	for(int x = -1; x <= 1; x++)
		for(int y = -1; y <= 1; y++)
			if(texture(shadowMap, thisFragment.xy + vec2(x,y) * texelSize).r > thisFragment.z) v += 1.0;
	return v/9.0;
}

void main() {
	if(isDebugMode == 1) {
		color = vec4(1);
		return;
	}
	vec3 thisFragment = vPosLightSpace.xyz / vPosLightSpace.w * 0.5 + 0.5;


	// underwater effect

	

    // the value for the sine has 2 inputs:
    // 1. the time, so that it animates.
    // 2. the y-row, so that ALL scanlines do not distort equally.
	float localtime = speed * time;
    float xAngle = time + fragCoords.y * ySineCycles;
    float yAngle = time + fragCoords.x * xSineCycles;

    vec2 distortOffset = 
        vec2(sin(xAngle), sin(yAngle)) * // amount of shearing
        vec2(xDistMag,yDistMag); // magnitude adjustment

	vec2 waterTexCoords = fragCoords + distortOffset; 	


	//color = texture(diffuseTexture, waterTexCoords);
	color = texture(diffuseTexture, vTexCoords);
	// color *= visibility(thisFragment);
	if(texture(shadowMap, thisFragment.xy).r < (thisFragment.z - DEPTH_BIAS))
		color *= 0.5;

}