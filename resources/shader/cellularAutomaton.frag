#version 330 core

#define EMPTY 0U
#define INSIDE_ROOM 1U
#define CORNER 2U
#define WALL 4U
#define TOP 8U
#define BOTTOM 16U
#define RIGHT 32U
#define LEFT 64U
#define INVALID 128U
#define SOURCE 256U
#define INFECTED 512U
#define OUTER_INFLUENCE 1024U

uniform vec2 pxsize;
uniform usampler2D inputGrid;

uniform ivec2 moveDirection;// each component in {-1,0,1}
uniform float BIRTH_THRESHOLD;// in [0,1]
uniform float DEATH_THRESHOLD;// in [0,1]
uniform float ROOM_NBORS_AHEAD_THRESHOLD;// in [0,1]
uniform int OUTER_INFL_NBORS_THRESHOLD;// integer from 0 to 8
uniform int DAMAGE_PER_CELL;// integer from 0 to 100

in vec2 pixel;
out uvec4 outputCell;

#define N 0
#define NE 1
#define E 2
#define SE 3
#define S 4
#define SW 5
#define W 6
#define NW 7
uvec2 neighborhood[8];

void lookupNeighborhood8();

int countNeighborsWithState(uint st);
ivec4 countNeighborsWithStateDirected(uint st);
int countNeighborsWithStateBetween(uint st1, uint st2);
ivec4 countNeighborsWithStateBetweenDirected(uint st1, uint st2);
int countNeighborsWithState(uint state, ivec2 start, ivec2 end);
ivec4 countNeighborsWithStateDirected(uint state, ivec2 start, ivec2 end);

uvec2 lookup(usampler2D s, vec2 c) {
	return texture(s,c).rg;
}

void setOutput(uint buildState, uint healthPoints) {
	outputCell = uvec4(buildState, healthPoints, 0, 0);
}

void life(uint state, int neighbors) {
}

void main() {
	
	// Look up the states of myself and of my neighbors
	uvec2 cell = lookup(inputGrid, pixel);
	lookupNeighborhood8();

	uint state = cell.r;
	int neighbors = countNeighborsWithState(state);
	
	if(state == EMPTY && neighbors == 3) {
		setOutput(INFECTED, 100U);
	}
	else if(state == INFECTED && neighbors < 2) {
		setOutput(EMPTY, 100U);
	}
	else if(state == INFECTED && neighbors > 3) {
		setOutput(EMPTY, 100U);
	}
	else {
		setOutput(state, 100U);
	}
}


// Helper functions

void lookupNeighborhood8() {
	neighborhood[N] = lookup(inputGrid, vec2(pixel.x, pixel.y+pxsize.y)).rg;
	neighborhood[NE] = lookup(inputGrid, vec2(pixel.x+pxsize.x, pixel.y+pxsize.y)).rg;
	neighborhood[E] = lookup(inputGrid, vec2(pixel.x+pxsize.x, pixel.y)).rg;
	neighborhood[SE] = lookup(inputGrid, vec2(pixel.x+pxsize.x, pixel.y-pxsize.y)).rg;
	neighborhood[S] = lookup(inputGrid, vec2(pixel.x, pixel.y-pxsize.y)).rg;
	neighborhood[SW] = lookup(inputGrid, vec2(pixel.x-pxsize.x, pixel.y-pxsize.y)).rg;
	neighborhood[W] = lookup(inputGrid, vec2(pixel.x-pxsize.x, pixel.y)).rg;
	neighborhood[NW] = lookup(inputGrid, vec2(pixel.x-pxsize.x, pixel.y+pxsize.y)).rg;
}

int countNeighborsWithState(uint st) {
	int cnt = 0;
	for(int i = 0; i < 8; i++) {
		if(neighborhood[i].r == st) cnt++;
	}
	return cnt;
}

ivec4 countNeighborsWithStateDirected(uint st) {
	// x = number of neighbors in positive x direction
	// y = neighbors in positive y
	// z = neighbors in negative x
	// w = neighbors in negative y
	ivec4 cnt = ivec4(0);
	if(neighborhood[NE].r == st) {
		cnt.x++;
		cnt.y++;
	}
	if(neighborhood[SE].r == st) {
		cnt.x++;
		cnt.w++;
	}
	if(neighborhood[SW].r == st) {
		cnt.z++;
		cnt.w++;
	}
	if(neighborhood[NW].r == st) {
		cnt.z++;
		cnt.y++;
	}
	if(neighborhood[N].r == st) cnt.y++;
	if(neighborhood[E].r == st) cnt.x++;
	if(neighborhood[S].r == st) cnt.w++;
	if(neighborhood[W].r == st) cnt.z++;
	return cnt;
}

int countNeighborsWithStateBetween(uint st1, uint st2) {
	int cnt = 0;
	for(int i = 0; i < 8; i++) {
		if(neighborhood[i].r >= st1 && neighborhood[i].r <= st2)
			cnt++;
	}
	return cnt;
}

ivec4 countNeighborsWithStateBetweenDirected(uint st1, uint st2) {
	// x = number of neighbors in positive x direction
	// y = neighbors in positive y
	// z = neighbors in negative x
	// w = neighbors in negative y
	ivec4 cnt = ivec4(0);
	if(neighborhood[NE].r >= st1 && neighborhood[NE].r <= st2) {
		cnt.x++;
		cnt.y++;
	}
	if(neighborhood[SE].r >= st1 && neighborhood[SE].r <= st2) {
		cnt.x++;
		cnt.w++;
	}
	if(neighborhood[SW].r >= st1 && neighborhood[SW].r <= st2) {
		cnt.z++;
		cnt.w++;
	}
	if(neighborhood[NW].r >= st1 && neighborhood[NW].r <= st2) {
		cnt.z++;
		cnt.y++;
	}
	if(neighborhood[N].r >= st1 && neighborhood[N].r <= st2)
		cnt.y++;
	if(neighborhood[E].r >= st1 && neighborhood[E].r <= st2)
		cnt.x++;
	if(neighborhood[S].r >= st1 && neighborhood[S].r <= st2)
		cnt.w++;
	if(neighborhood[W].r >= st1 && neighborhood[W].r <= st2)
		cnt.z++;
	return cnt;
}

int countNeighborsWithState(uint state, ivec2 start, ivec2 end) {
	int cnt = 0;
	for(int x = start.x; x <= end.x; x += 1) {
		for(int y = start.y; y <= end.y; y += 1) {
			if(x==0 && y==0) continue;
			if(lookup(inputGrid, pixel + vec2(x*pxsize.x, y*pxsize.y)).r == state) {
				cnt++;
			}
		}
	}
	return cnt;
}

ivec4 countNeighborsWithStateDirected(uint state, ivec2 start, ivec2 end) {
	// x = number of neighbors in positive x direction
	// y = neighbors in positive y
	// z = neighbors in negative x
	// w = neighbors in negative y
	ivec4 cnt = ivec4(0);
	for(int x = start.x; x <= end.x; x += 1) {
		for(int y = start.y; y <= end.y; y += 1) {
			uint neighborState = lookup(inputGrid, pixel + vec2(x*pxsize.x, y*pxsize.y)).r;
			if(neighborState != state) continue;
			if(x > 0) cnt.x++;
			else if(x < 0) cnt.z++;
			if(y > 0) cnt.y++;
			else if(y < 0) cnt.w++;
		}
	}
	return cnt;
}
