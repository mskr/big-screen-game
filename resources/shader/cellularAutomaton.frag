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

uniform vec2 pxsize;
uniform isampler2D inputGrid;

uniform ivec2 moveDirection;// each component in {-1,0,1}
uniform float BIRTH_THRESHOLD;// in [0,1]
uniform float DEATH_THRESHOLD;// in [0,1]
uniform float ROOM_NBORS_AHEAD_THRESHOLD;// in [0,1]
uniform int OUTER_INFL_NBORS_THRESHOLD;// integer from 0 to 8
uniform int DAMAGE_PER_CELL;// integer from 0 to 100

in vec2 pixel;
out ivec2 outputCell;

#define N 0
#define NE 1
#define E 2
#define SE 3
#define S 4
#define SW 5
#define W 6
#define NW 7
ivec2 neighborhood[8];

void lookupNeighborhood8() {
	neighborhood[N] = texture(inputGrid, vec2(pixel.x, pixel.y+pxsize.y)).rg;
	neighborhood[NE] = texture(inputGrid, vec2(pixel.x+pxsize.x, pixel.y+pxsize.y)).rg;
	neighborhood[E] = texture(inputGrid, vec2(pixel.x+pxsize.x, pixel.y)).rg;
	neighborhood[SE] = texture(inputGrid, vec2(pixel.x+pxsize.x, pixel.y-pxsize.y)).rg;
	neighborhood[S] = texture(inputGrid, vec2(pixel.x, pixel.y-pxsize.y)).rg;
	neighborhood[SW] = texture(inputGrid, vec2(pixel.x-pxsize.x, pixel.y-pxsize.y)).rg;
	neighborhood[W] = texture(inputGrid, vec2(pixel.x-pxsize.x, pixel.y)).rg;
	neighborhood[NW] = texture(inputGrid, vec2(pixel.x-pxsize.x, pixel.y+pxsize.y)).rg;
}

int countNeighborsWithState(int st) {
	int cnt = 0;
	for(int i = 0; i < 8; i++) {
		if(neighborhood[i].r == st) cnt++;
	}
	return cnt;
}

ivec4 countNeighborsWithStateDirected(int st) {
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

int countNeighborsWithStateBetween(int st1, int st2) {
	int cnt = 0;
	for(int i = 0; i < 8; i++) {
		if(neighborhood[i].r >= st1 && neighborhood[i].r <= st2)
			cnt++;
	}
	return cnt;
}

ivec4 countNeighborsWithStateBetweenDirected(int st1, int st2) {
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

int applyMoveRule(int st, ivec4 nbors, ivec2 movDir) {
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

int applySplitRule(int st, ivec4 roomNbors, int outerInflNbors, ivec2 movDir) {
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

int applyDamageRule(int st, int hp, int outerInflNbors, int roomNbors) {
	if(st >= BSTATE_INSIDE_ROOM && st <= BSTATE_WALL_BOTTOM) {
		return hp - outerInflNbors * DAMAGE_PER_CELL;
	}
	else if(st == BSTATE_OUTER_INFLUENCE) {
		return hp - roomNbors * DAMAGE_PER_CELL;
	}
	else return hp;
}

// ALTERNATIVE RULES (tried to conserve number of cells)
int moveCollision(int st) {
	int stBehind = texture(inputGrid, pixel - vec2(moveDirection) * pxsize).r;
	int stAhead = texture(inputGrid, pixel + vec2(moveDirection) * pxsize).r;
	// Movement
	if(st==BSTATE_EMPTY && stBehind==BSTATE_OUTER_INFLUENCE) // rule
		return BSTATE_OUTER_INFLUENCE;
	if(st==BSTATE_OUTER_INFLUENCE && stBehind==BSTATE_EMPTY) // counter-rule
		return BSTATE_EMPTY;
	// Collision
	if(stAhead>=BSTATE_INSIDE_ROOM && stAhead<=BSTATE_INVALID) {
		//TODO cells in the middle get lost
		// try to propagate collision information in extra channel
		int stLeft = texture(inputGrid, pixel + vec2(-moveDirection.y,moveDirection.x) * pxsize).r;
		int stRight = texture(inputGrid, pixel + vec2(moveDirection.y,-moveDirection.x) * pxsize).r;
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
	ivec2 cell = texture(inputGrid, pixel).rg;
	lookupNeighborhood8();
	ivec4 outerInflNborsDir = countNeighborsWithStateDirected(BSTATE_OUTER_INFLUENCE);
	int newState = applyMoveRule(cell.r, outerInflNborsDir, moveDirection);
	int outerInflNbors = outerInflNborsDir.x+outerInflNborsDir.y+outerInflNborsDir.z+outerInflNborsDir.w;
	ivec4 roomNborsDir = countNeighborsWithStateBetweenDirected(BSTATE_INSIDE_ROOM, BSTATE_WALL_BOTTOM);
	int roomNbors = roomNborsDir.x+roomNborsDir.y+roomNborsDir.z+roomNborsDir.w;
	newState = applySplitRule(newState, roomNborsDir, outerInflNbors, moveDirection);
	int newHealth = applyDamageRule(cell.r, cell.g, outerInflNbors, roomNbors);
	if(newHealth <= 0) newState = BSTATE_EMPTY;
	if(newState == BSTATE_EMPTY) newHealth = 100; //TODO Dynamic cells die too slow. Better track cells and their health
	outputCell = ivec2(newState, newHealth);
}