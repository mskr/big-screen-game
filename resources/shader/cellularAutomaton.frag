#version 330 core

/* Texture holding minimal version of grid state
(build state and health as 32 bit unsigned integers) */
uniform usampler2D inputGrid;

/* Build state bits */
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

/* Max/min health (should match GridCell::MAX/MIN_HEALTH) */
#define MAX_HEALTH 100U
#define MIN_HEALTH 0U

/* Pixel position aka. cell position in input grid */
in vec2 pixel;

/* Pixel size aka. cell size (for stepping to neighbors) */
uniform vec2 pxsize;

/* Output state, i.e. accumulated effect on the current cell */
out uvec4 outputCell; // written to another texture

/* Function for lookup current neighborhood from the input grid */
void lookupNeighborhood8();
uvec2 neighborhood[8]; // neighborhood states are returned here
#define N 0 // indices of neighbors for each direction
#define NE 1
#define E 2
#define SE 3
#define S 4
#define SW 5
#define W 6
#define NW 7

/* Functions for analyzing neighborhood configurations */
int countNeighborsWithState(uint st);
ivec4 countNeighborsWithStateDirected(uint st);
int countNeighborsInRangeWithState(uint state, ivec2 start, ivec2 end);
ivec4 countNeighborsInRangeWithStateDirected(uint state, ivec2 start, ivec2 end);
bool isStateInNeighborhood(uint state);
uint getFluidGradientInDirection(int dir); // dir is one of above defines (N,NE...)

/* Function for lookup state in given grid at given cell */
uvec2 lookup(usampler2D grid, vec2 cell) {
    return texture(grid,cell).rg;
}

/* Function for saving the result to the other texture */
void setOutput(uint buildState, uint healthPoints) {
    outputCell = uvec4(buildState, healthPoints, 0, 0);
}

/* Simulation parameters */
/* adapted from http://www2.econ.iastate.edu/tesfatsi/SandPileModel.pdf */
// consider 4 or 8 neighbors?
#define NUM_DIRECTIONS 4
// flow direction rotates per time step
uniform ivec2 FLOW_DIRECTION;
// threshold where the fluid begins to flow
uniform int CRITICAL_VALUE;
// amount of fluid a lower cell receives from collapsing neighbor
uniform uint FLOW_SPEED;




/******************** MAIN *************************/
void main() {
    uvec2 cell = lookup(inputGrid, pixel);
    uvec2 nbor_cell = lookup(inputGrid, pixel + 
        vec2(-FLOW_DIRECTION.x * pxsize.x, -FLOW_DIRECTION.y * pxsize.y));

    // BACKGROUND STORY:
    // a room got attacked and is now infected
    // through a source in a wall fluid comes in
    // the fluid slowly damages the room
    // the player tries to repair...

    uint bstate = cell[0];
    uint health = cell[1];
    uint fluid = MAX_HEALTH - health;

    uint nbor_bstate = nbor_cell[0];
    uint nbor_health = nbor_cell[1];
    uint nbor_fluid = MAX_HEALTH - nbor_health;

    // fluid gradient: positive=>incoming, negative=>outgoing flow
    int fluid_gradient = int(nbor_fluid - fluid);

    // CASE 1: cell is SOURCE (& WALL)
    if((bstate & SOURCE) > 0U) {
        // constantly decrease health, while player repairs
        setOutput(bstate | INFECTED, clamp(health - FLOW_SPEED, MIN_HEALTH, MAX_HEALTH));
        return;
    }

    // CASE 2: cell is INFECTED (& INSIDE_ROOM)
    else if((bstate & INFECTED) > 0U) {
        // compute flow assuming that fluid flows "down hill"
        if(fluid_gradient > CRITICAL_VALUE) {
            setOutput(bstate, health - FLOW_SPEED);
            return;
        }
        else if(fluid_gradient < -CRITICAL_VALUE) {
            setOutput(bstate, health + FLOW_SPEED);
            return;
        }
    }

    // CASE 3: cell is INSIDE_ROOM
    else if((bstate & INSIDE_ROOM) > 0U) {
        // decide whether this cell gets infected
        if(fluid_gradient > CRITICAL_VALUE) {
            setOutput(bstate | INFECTED, health);
            return;
        }
    }

    //CASE 4: cell is EMPTY or far from INFECTED
    setOutput(bstate, health);
}



/******************** HELPER FUNCTION IMPLEMENTATIONS *************************/

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

bool isStateInNeighborhood(uint state) {
    for(int i = 0; i < 8; i++) {
        if((neighborhood[i].r & state) > 0U)
            return true;
    }
    return false;
}

int countNeighborsWithState(uint st) {
    int cnt = 0;
    for(int i = 0; i < 8; i++) {
        if((neighborhood[i].r & st) > 0U) cnt++;
    }
    return cnt;
}

ivec4 countNeighborsWithStateDirected(uint st) {
    // x = number of neighbors in positive x direction
    // y = neighbors in positive y
    // z = neighbors in negative x
    // w = neighbors in negative y
    ivec4 cnt = ivec4(0);
    if((neighborhood[NE].r & st) > 0U) {
        cnt.x++;
        cnt.y++;
    }
    if((neighborhood[SE].r & st) > 0U) {
        cnt.x++;
        cnt.w++;
    }
    if((neighborhood[SW].r & st) > 0U) {
        cnt.z++;
        cnt.w++;
    }
    if((neighborhood[NW].r & st) > 0U) {
        cnt.z++;
        cnt.y++;
    }
    if((neighborhood[N].r & st) > 0U) cnt.y++;
    if((neighborhood[E].r & st) > 0U) cnt.x++;
    if((neighborhood[S].r & st) > 0U) cnt.w++;
    if((neighborhood[W].r & st) > 0U) cnt.z++;
    return cnt;
}

int countNeighborsInRangeWithState(uint state, ivec2 start, ivec2 end) {
    // start/end are distances from current cell in units of cells
    // distance is given in x and y direction
    // negative distance means left/below
    // positive distance means right/above
    int cnt = 0;
    for(int x = start.x; x <= end.x; x += 1) {
        for(int y = start.y; y <= end.y; y += 1) {
            if(x==0 && y==0) continue;
            uvec2 cell = lookup(inputGrid, pixel + vec2(x*pxsize.x, y*pxsize.y));
            if((cell.r & state) > 0U) {
                cnt++;
            }
        }
    }
    return cnt;
}

ivec4 countNeighborsInRangeWithStateDirected(uint state, ivec2 start, ivec2 end) {
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