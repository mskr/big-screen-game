#version 330 core

#define BSTATE_EMPTY 0
#define BSTATE_INSIDE_ROOM 1
#define BSTATE_LEFT_UPPER_CORNER 2
#define BSTATE_RIGHT_UPPER_CORNER 3
#define BSTATE_LEFT_LOWER_CORNER 4
#define BSTATE_RIGHT_LOWER_CORNER 5
#define BSTATE_WALL_LEFT 6
#define BSTATE_WALL_RIGHT 7
#define BSTATE_WALL_TOP 8
#define BSTATE_WALL_BOTTOM 9
#define BSTATE_INVALID 10
#define BSTATE_OUTER_INFLUENCE 11

in vec2 pixel;
uniform vec2 pxsize;
uniform isampler2D inputGrid;
out ivec2 outputCell;

int countNeighborsWithState(int state, ivec2 start, ivec2 end) {
	int cnt = 0;
	for(int x = start.x; x <= end.x; x += 1) {
		for(int y = start.y; y <= end.y; y += 1) {
			if(x==0 && y==0) continue;
			if(texture(inputGrid, pixel + vec2(x*pxsize.x, y*pxsize.y)).r == state) {
				cnt++;
			}
		}
	}
	return cnt;
}

ivec4 countNeighborsWithStateDirected(int state, ivec2 start, ivec2 end) {
	// x = number of neighbors in positive x direction
	// y = neighbors in positive y
	// z = neighbors in negative x
	// w = neighbors in negative y
	ivec4 cnt = ivec4(0);
	for(int x = start.x; x <= end.x; x += 1) {
		for(int y = start.y; y <= end.y; y += 1) {
			int neighborState = texture(inputGrid, pixel + vec2(x*pxsize.x, y*pxsize.y)).r;
			if(neighborState != state) continue;
			if(x > 0) cnt.x++;
			else if(x < 0) cnt.z++;
			if(y > 0) cnt.y++;
			else if(y < 0) cnt.w++;
		}
	}
	return cnt;
}

void life(int state, int neighbors) {
	if(state == BSTATE_EMPTY && neighbors == 3) {
		outputCell = ivec2(BSTATE_OUTER_INFLUENCE, 100);
	}
	else if(state == BSTATE_OUTER_INFLUENCE && neighbors < 2) {
		outputCell = ivec2(BSTATE_EMPTY, 100);
	}
	else if(state == BSTATE_OUTER_INFLUENCE && neighbors > 3) {
		outputCell = ivec2(BSTATE_EMPTY, 100);
	}
	else {
		outputCell = ivec2(state, 100);
	}
}

void outerInfluence(int state, ivec4 neighbors, ivec2 movingDir) {
	if(state == BSTATE_EMPTY) {
		if(movingDir.x > 0 && neighbors.z > 0)
			outputCell = ivec2(BSTATE_OUTER_INFLUENCE, 100);
		else if(movingDir.x < 0 && neighbors.x > 0)
			outputCell = ivec2(BSTATE_OUTER_INFLUENCE, 100);
		else
			outputCell = ivec2(state, 100);
		if(movingDir.y > 0 && neighbors.w > 0)
			outputCell = ivec2(BSTATE_OUTER_INFLUENCE, 100);
		else if(movingDir.y < 0 && neighbors.y > 0)
			outputCell = ivec2(BSTATE_OUTER_INFLUENCE, 100);
		else
			outputCell = ivec2(state, 100);
	}
	else if(state == BSTATE_OUTER_INFLUENCE) {
		if(movingDir.x > 0 && neighbors.z == 0)
			outputCell = ivec2(BSTATE_EMPTY, 100);
		else if(movingDir.x < 0 && neighbors.x == 0)
			outputCell = ivec2(BSTATE_EMPTY, 100);
		else
			outputCell = ivec2(state, 100);
		if(movingDir.y > 0 && neighbors.w == 0)
			outputCell = ivec2(BSTATE_EMPTY, 100);
		else if(movingDir.y < 0 && neighbors.y == 0)
			outputCell = ivec2(BSTATE_EMPTY, 100);
		else
			outputCell = ivec2(state, 100);
	}
	else {
		outputCell = ivec2(state, 100);
	}
}

int applyMoveRules(int st, ivec4 nbors, ivec2 movDir) {
	if (st==BSTATE_EMPTY && movDir.x>0 && nbors.z>1) return BSTATE_OUTER_INFLUENCE;
	else if (st==BSTATE_OUTER_INFLUENCE && movDir.x>0 && nbors.z==0) return BSTATE_EMPTY;
	else return st;
}

void main() {
	int state = texture(inputGrid, pixel).r;
	int neighbors = countNeighborsWithState(BSTATE_OUTER_INFLUENCE, ivec2(-1,-1), ivec2(1,1));
	ivec4 neighborsDirected = countNeighborsWithStateDirected(BSTATE_OUTER_INFLUENCE, ivec2(-1,-1), ivec2(1,1));
	outputCell = ivec2(applyMoveRules(state, neighborsDirected, ivec2(1,0)), 100);
}