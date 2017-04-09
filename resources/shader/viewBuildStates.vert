#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in int buildState;
layout(location = 2) in int healthPoints;

uniform mat4 MVP;

flat out vec2 fPosition;
flat out int fBuildState;
flat out int fHealthPoints;

void main()
{
	gl_PointSize = 4.0;
	gl_Position = MVP * vec4(position, 0.0, 1.0);
	fPosition = position;
	fBuildState = buildState;
	fHealthPoints = healthPoints;
}