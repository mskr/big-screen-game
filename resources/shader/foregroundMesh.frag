#version 330 core

#define BSTATE_OUTER_INFLUENCE 11

uniform sampler2D diffuseTexture;

in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoords;
flat in int vHealth;

out vec4 color;

flat in int buildState;

in vec4 c;

void main()
{
    vec3 lightDir = normalize(vPosition - vec3(-10.0f, -10.0f, -10.0f));

    float NdotL = clamp(dot(lightDir, normalize(vNormal)), 0.0f, 1.0f);
    vec3 texColor = texture(diffuseTexture, vTexCoords).rgb;
    float healthNormalized = float(vHealth)/100;
    if(buildState==BSTATE_OUTER_INFLUENCE)
    	color = vec4(1,1,1,1) * healthNormalized;
	else
    	color = vec4(vNormal.x,vNormal.y,vNormal.z,1) * healthNormalized;
}