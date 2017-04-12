#version 330 core

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoords;
in vec4 vPosLightSpace;

out vec4 color;

bool testShadow(vec4 positionLightSpace) {
	vec3 ndc = positionLightSpace.xyz / positionLightSpace.w;
	ndc = ndc * 0.5 + 0.5;
	// ndc.y = 1 - ndc.y;
	ndc.x = 1 - ndc.x;
	float closestDepth = texture(shadowMap, ndc.xy).r;

	/* PCF */
	// float shadow = 0.0;
	// vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	// for(int i = -1; i <= 1; i += 1) {
	// 	for(int j = -1; j <= 1; j+= 1) {
	// 		float closestDepth = texture(ShadowMap, ndc.xy + vec2(i,j) * texelSize).r;
	// 		shadow += ((ndc.z - 0.0001) > closestDepth) ? 0.0 : 1.0;
	// 	}
	// }
	// return shadow/9.0;

	float currentDepth = ndc.z;
	return closestDepth < (currentDepth-0.0001);
}

const float DEPTH_BIAS = 0.00001;

void main() {
	vec3 thisFragment = vPosLightSpace.xyz / vPosLightSpace.w * 0.5 + 0.5;
	color = texture(diffuseTexture, vTexCoords);
	if(texture(shadowMap, thisFragment.xy).r < (thisFragment.z - DEPTH_BIAS))
		color *= 0.5;
}