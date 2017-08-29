#include "InnerInfluence.h"

InnerInfluence::InnerInfluence(AutomatonGrid* grid, double transition_time) :
	GPUCellularAutomaton(grid, transition_time), num_transitions_(0), NUM_DIRECTIONS_(8),
    FLOW_DIRECTION(new glm::ivec2[NUM_DIRECTIONS_]{
        { 1, -1 },
        { 1, 0 },
        { 1, 1 },
        { 0, 1 },
        { -1, 1 },
        { -1, 0 },
        { -1, -1 },
        { 0, -1 }
    }),
    FLOW_SPEED(4),
    CRITICAL_VALUE(12)
{
}

InnerInfluence::~InnerInfluence() {
    delete[] FLOW_DIRECTION;
}

void InnerInfluence::init(viscom::GPUProgramManager mgr) {
	GPUCellularAutomaton::init(mgr);
	uloc_FLOW_DIRECTION = shader_->getUniformLocation("FLOW_DIRECTION");
	uloc_FLOW_SPEED = shader_->getUniformLocation("FLOW_SPEED");
	uloc_CRITICAL_VALUE = shader_->getUniformLocation("CRITICAL_VALUE");
}

bool InnerInfluence::transition(double time) {
	if (GPUCellularAutomaton::isInitialized()) {
		glUseProgram(shader_->getProgramId());
		glUniform2i(uloc_FLOW_DIRECTION, 
            FLOW_DIRECTION[num_transitions_ % NUM_DIRECTIONS_].x,
            FLOW_DIRECTION[num_transitions_ % NUM_DIRECTIONS_].y);
		glUniform1ui(uloc_FLOW_SPEED, FLOW_SPEED);
		glUniform1i(uloc_CRITICAL_VALUE, CRITICAL_VALUE);
        if (GPUCellularAutomaton::transition(time)) {
            /*
            // rotate flow direction 90 degrees clockwise
            glm::ivec2 tmp = FLOW_DIRECTION;
            tmp.x = 0 * FLOW_DIRECTION.x + 1 * FLOW_DIRECTION.y;
            tmp.y = -1 * FLOW_DIRECTION.x + 0 * FLOW_DIRECTION.y;
            FLOW_DIRECTION = tmp;
            */
            num_transitions_++;
            return true;
        }
	}
    return false;
}