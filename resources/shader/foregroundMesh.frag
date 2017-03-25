#version 330 core

uniform sampler2D diffuseTexture;

in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoords;
flat in int vHealth;

out vec4 color;

void main()
{
    vec3 lightDir = normalize(vPosition - vec3(-10.0f, -10.0f, -10.0f));

    float NdotL = clamp(dot(lightDir, normalize(vNormal)), 0.0f, 1.0f);
    vec3 texColor = texture(diffuseTexture, vTexCoords).rgb;
    float healthNormalized = float(vHealth)/100;
    color = vec4(vNormal.x,vNormal.y,vNormal.z,1) * vec4(1-healthNormalized, healthNormalized, healthNormalized, 1);//vec4(1,1,1,1)*NdotL;//vec4(texColor * NdotL, 1.0f);
}