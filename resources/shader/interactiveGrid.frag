#version 330 core

flat in vec2 fPosition;
flat in int fBuildState;
flat in int fHealthPoints;

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
	// Left Upper Corner
	else if(fBuildState == 2) color = vec4(1,.5,.5,1); // red 
	// Right Upper Corner
	else if(fBuildState == 3) color = vec4(.5,0,0,1); // dark-red
	// Left Lower Corner
	else if(fBuildState == 4) color = vec4(.5,0,.5,1); // dark-magenta
	// Right Lower Corner
	else if(fBuildState == 5) color = vec4(1,0,1,1); // magenta
	// Wall Left
	else if(fBuildState == 6) color = vec4(1,1,0,1); // very bright yellow
	// Wall Right
	else if(fBuildState == 7) color = vec4(1,1,0,1)*0.8; // bright yellow
	// Wall Top
	else if(fBuildState == 8) color = vec4(1,1,0,1)*0.6; // dark yellow
	// Wall Bottom
	else if(fBuildState == 9) color = vec4(1,1,0,1)*0.4; // very dark yellow
	// Invalid Build State (room too small)
	else if(fBuildState == 10) color = vec4(1,0,0,1); // red
	// Outer Influence
	else if(fBuildState == 11) color = vec4(0,0,.5,1); // dark blue
	else color = vec4(.5,.5,.5,1); // gray
}