#version 330 core

const uint BSTATE_EMPTY = 0U;
const uint BSTATE_INSIDE_ROOM = 1U;
const uint BSTATE_LEFT_UPPER_CORNER = 2U;
const uint BSTATE_RIGHT_UPPER_CORNER = 3U;
const uint BSTATE_LEFT_LOWER_CORNER = 4U;
const uint BSTATE_RIGHT_LOWER_CORNER = 5U;
const uint BSTATE_WALL_LEFT = 6U;
const uint BSTATE_WALL_RIGHT = 7U;
const uint BSTATE_WALL_TOP = 8U;
const uint BSTATE_WALL_BOTTOM = 9U;
const uint BSTATE_INVALID = 10U;
const uint BSTATE_OUTER_INFLUENCE = 11U;

uniform vec2 pxsize;
uniform sampler2D inputGrid;

uniform ivec2 moveDirection;// each component in {-1,0,1}
uniform float BIRTH_THRESHOLD;// in [0,1]
uniform float DEATH_THRESHOLD;// in [0,1]
uniform float ROOM_NBORS_AHEAD_THRESHOLD;// in [0,1]
uniform int OUTER_INFL_NBORS_THRESHOLD;// integer from 0 to 8
uniform int DAMAGE_PER_CELL;// integer from 0 to 100

in vec2 pixel;
out vec4 outputCell;

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

uvec2 lookup(sampler2D s, vec2 c) {
	return uvec2(texture(s,c).rg * 255); // convert UNORM to uint
}

void setOutput(uint buildState, uint healthPoints) {
	outputCell = vec4(float(buildState)/255.0, float(healthPoints)/255.0, 0, 0); // convert uint to UNORM
}

void life(uint state, int neighbors) {
	if(state == BSTATE_EMPTY && neighbors == 3) {
		setOutput(BSTATE_OUTER_INFLUENCE, 100U);
	}
	else if(state == BSTATE_OUTER_INFLUENCE && neighbors < 2) {
		setOutput(BSTATE_EMPTY, 100U);
	}
	else if(state == BSTATE_OUTER_INFLUENCE && neighbors > 3) {
		setOutput(BSTATE_EMPTY, 100U);
	}
	else {
		setOutput(state, 100U);
	}
}

///////////
// Rules //
///////////
// 1) Move:
//    - Neighbors in opposite moving direction are
//      considered to follow behind the current cell.
//    - Outer influence cell is born, if enough
//      other outer influence cells follow behind it
//      (more than BIRTH_THRESHOLD).
//    - Outer influence cell dies, if too few other
//      outer influence cells follow behind it
//      (fewer than DEATH_THRESHOLD).
// 2) Split:
//    - As response to a collision with a room, outer
//      influence body "splits".
//    - Split rule takes the new state after the move rule.
//    - Outer influence cell is born next to at least
//      OUTER_INFL_NBORS_THRESHOLD other outer influence
//      cells, if enough room cells are ahead (more than
//      ROOM_NBORS_AHEAD_THRESHOLD).
// 3) Damage:
//    - Room cell health decreases by the value of
//      DAMAGE_PER_CELL for each outer influence neighbor.
//    - Outer influence cell health decreases by the
//      value of DAMAGE_PER_CELL for each room neighbor.

uint applyMoveRule(uint st, ivec4 nbors, ivec2 movDir) {
	int nborsBehind = 0;
	if(movDir.x!=0) {
		nborsBehind += (movDir.x>0) ? nbors.z : nbors.x;
	}
	if(movDir.y!=0) {
		nborsBehind += (movDir.y>0) ? nbors.w : nbors.y;
	}
	float nborsBehindNormalized = nborsBehind;
	if(movDir.x!=0 && movDir.y!=0) {
		nborsBehindNormalized /= 6.0;
	}
	else {
		nborsBehindNormalized /= 3.0;
	}
	if (st==BSTATE_EMPTY && nborsBehindNormalized>BIRTH_THRESHOLD)
		return BSTATE_OUTER_INFLUENCE; // birth
	else if (st==BSTATE_OUTER_INFLUENCE && nborsBehindNormalized<DEATH_THRESHOLD)
		return BSTATE_EMPTY; // death
	else
		return st; // remain dead/alive
}

uint applySplitRule(uint st, ivec4 roomNbors, int outerInflNbors, ivec2 movDir) {
	if(st!=BSTATE_EMPTY) return st;
	int roomNborsAhead = 0;
	if(movDir.x!=0) {
		roomNborsAhead += (movDir.x>0) ? roomNbors.x : roomNbors.z;
	}
	if(movDir.y!=0) {
		roomNborsAhead += (movDir.y>0) ? roomNbors.y : roomNbors.w;
	}
	float roomNborsAheadNormalized = roomNborsAhead;
	if(movDir.x!=0 && movDir.y!=0) roomNborsAheadNormalized /= 6.0;
	else roomNborsAheadNormalized /= 3.0;
	if(roomNborsAheadNormalized>ROOM_NBORS_AHEAD_THRESHOLD
		&& outerInflNbors>=OUTER_INFL_NBORS_THRESHOLD) return BSTATE_OUTER_INFLUENCE;
	else return st;
}

uint applyDamageRule(uint st, uint hp, int outerInflNbors, int roomNbors) {
	int signedHP = int(hp);
	if(st >= BSTATE_INSIDE_ROOM && st <= BSTATE_WALL_BOTTOM) {
		return uint(clamp(signedHP - outerInflNbors * DAMAGE_PER_CELL, 0, 100));
	}
	else if(st == BSTATE_OUTER_INFLUENCE) {
		return uint(clamp(signedHP - roomNbors * DAMAGE_PER_CELL, 0, 100));
	}
	else return hp;
}

// ALTERNATIVE RULES (tried to conserve number of cells)
uint moveCollision(uint st) {
	uint stBehind = lookup(inputGrid, pixel - vec2(moveDirection) * pxsize).r;
	uint stAhead = lookup(inputGrid, pixel + vec2(moveDirection) * pxsize).r;
	// Movement
	if(st==BSTATE_EMPTY && stBehind==BSTATE_OUTER_INFLUENCE) // rule
		return BSTATE_OUTER_INFLUENCE;
	if(st==BSTATE_OUTER_INFLUENCE && stBehind==BSTATE_EMPTY) // counter-rule
		return BSTATE_EMPTY;
	// Collision
	if(stAhead>=BSTATE_INSIDE_ROOM && stAhead<=BSTATE_INVALID) {
		//TODO cells in the middle get lost
		// try to propagate collision information in extra channel
		uint stLeft = lookup(inputGrid, pixel + vec2(-moveDirection.y,moveDirection.x) * pxsize).r;
		uint stRight = lookup(inputGrid, pixel + vec2(moveDirection.y,-moveDirection.x) * pxsize).r;
		if(st==BSTATE_OUTER_INFLUENCE && (stLeft==BSTATE_EMPTY || stRight==BSTATE_EMPTY))
			return BSTATE_EMPTY;
		if(st==BSTATE_EMPTY && (stLeft==BSTATE_OUTER_INFLUENCE || stRight==BSTATE_OUTER_INFLUENCE))
			return BSTATE_OUTER_INFLUENCE;
		if(st==BSTATE_EMPTY && (stLeft==BSTATE_OUTER_INFLUENCE && stRight==BSTATE_OUTER_INFLUENCE)) {
			//TODO conflict in empty cell: two outer influence cells want to move here
			// But can only set one, so we lose one cell (not conservative)
			return BSTATE_OUTER_INFLUENCE;
		}
		return st;
	}
	return st;
}

void main() {
	
	// Look up the states of myself and of my neighbors
	uvec2 cell = lookup(inputGrid, pixel);
	lookupNeighborhood8();

	// Count specific configurations of states
	ivec4 outerInflNborsDir = countNeighborsWithStateDirected(BSTATE_OUTER_INFLUENCE);
	ivec4 roomNborsDir = countNeighborsWithStateBetweenDirected(BSTATE_INSIDE_ROOM, BSTATE_WALL_BOTTOM);
	int outerInflNbors = countNeighborsWithState(BSTATE_OUTER_INFLUENCE);
	int roomNbors = countNeighborsWithStateBetween(BSTATE_INSIDE_ROOM, BSTATE_WALL_BOTTOM);

	// Apply the rules
	uint newState = applyMoveRule(cell.r, outerInflNborsDir, moveDirection);
	newState = applySplitRule(newState, roomNborsDir, outerInflNbors, moveDirection);
	uint newHealth = applyDamageRule(cell.r, cell.g, outerInflNbors, roomNbors);

	// If cell died, it is replaced with empty cell
	if(newHealth == 0U)
		newState = BSTATE_EMPTY;

	// Ensure that new and old empty cells always have 100 HP
	if(newState == BSTATE_EMPTY)
		newHealth = 100U; //TODO Dynamic cells die too slow. Better track cells and their health
	
	setOutput(newState, newHealth);

	// Can also play conways game of life :)
	// life(cell.r, outerInflNbors);
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
