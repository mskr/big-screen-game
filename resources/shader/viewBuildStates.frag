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
	if(fBuildState == EMPTY) color = vec4(1,1,1,1); // white
	// Inside Room
	else if(fBuildState == INSIDE_ROOM) color = vec4(0,1,0,1); // green
	// Left Upper Corner
	else if(fBuildState == (LEFT|CORNER|TOP)) color = vec4(1,.5,.5,1); // red 
	// Right Upper Corner
	else if(fBuildState == (RIGHT|CORNER|TOP)) color = vec4(.5,0,0,1); // dark-red
	// Left Lower Corner
	else if(fBuildState == (LEFT|CORNER|BOTTOM)) color = vec4(.5,0,.5,1); // dark-magenta
	// Right Lower Corner
	else if(fBuildState == (RIGHT|CORNER|BOTTOM)) color = vec4(1,0,1,1); // magenta
	// Wall Left
	else if(fBuildState == (WALL|LEFT)) color = vec4(1,1,0,1); // very bright yellow
	// Wall Right
	else if(fBuildState == (WALL|RIGHT)) color = vec4(1,1,0,1)*0.8; // bright yellow
	// Wall Top
	else if(fBuildState == (WALL|TOP)) color = vec4(1,1,0,1)*0.6; // dark yellow
	// Wall Bottom
	else if(fBuildState == (WALL|BOTTOM)) color = vec4(1,1,0,1)*0.4; // very dark yellow
	// Invalid Build State (room too small)
	else if(fBuildState == INVALID) color = vec4(1,0,0,1); // red
	// Inner influence
	else if(fBuildState == (INSIDE_ROOM|INFECTED)) color = vec4(.5,1,.5,1); // light green
	// Outer Influence
	else if(fBuildState == OUTER_INFLUENCE) color = vec4(0,0,.5,1); // dark blue
	else color = vec4(.5,.5,.5,1); // gray
	color = color * (float(fHealthPoints)/100.0);
}