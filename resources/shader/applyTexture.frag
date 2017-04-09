#version 330 core

uniform sampler2D diffuseTexture;

in vec3 vPosition;
in vec3 vNormal;
in vec2 vTexCoords;

out vec4 color;

void main() {
	color = texture(diffuseTexture, vTexCoords);
}