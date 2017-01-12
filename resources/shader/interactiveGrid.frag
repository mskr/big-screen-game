#version 330 core

flat in vec2 fPosition;
flat in int fBuildState;
flat in float fHealthPoints;

out vec4 color;

void main()
{
	/*
	* This fragment shader color-codes parts of rooms.
	* The BuildState of a cell stands for a part of a room.
	*/

	// Empty
	if(fBuildState == 0) color = vec4(1,1,1,1); // white
	// Inside Room
	else if(fBuildState == 1) color = vec4(0,1,0,1); // green
	// Wall
	else if(fBuildState == 2) color = vec4(1,1,0,1); // yellow
	// Left Upper Corner
	else if(fBuildState == 3) color = vec4(1,0,0,1); // red 
	// Right Upper Corner
	else if(fBuildState == 4) color = vec4(.5,0,0,1); // dark-red
	// Left Lower Corner
	else if(fBuildState == 5) color = vec4(.5,0,.5,1); // dark-magenta
	// Right Lower Corner
	else if(fBuildState == 6) color = vec4(1,0,1,1); // magenta
	else color = vec4(1,1,1,1); // white
}