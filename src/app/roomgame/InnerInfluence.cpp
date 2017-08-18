#include "InnerInfluence.h"

InnerInfluence::InnerInfluence(AutomatonGrid* grid, double transition_time) :
	GPUCellularAutomaton(grid, transition_time)
{
    FLOW_DIRECTION = glm::ivec2(-1, 0); // arbitrary start direction
    FLOW_SPEED = 4;
    CRITICAL_VALUE = 12;
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
		glUniform2i(uloc_FLOW_DIRECTION, FLOW_DIRECTION.x, FLOW_DIRECTION.y);
		glUniform1ui(uloc_FLOW_SPEED, FLOW_SPEED);
		glUniform1i(uloc_CRITICAL_VALUE, CRITICAL_VALUE);
        if (GPUCellularAutomaton::transition(time)) {
            // rotate flow direction 90 degrees clockwise
            glm::ivec2 tmp = FLOW_DIRECTION;
            tmp.x = 0 * FLOW_DIRECTION.x + 1 * FLOW_DIRECTION.y;
            tmp.y = -1 * FLOW_DIRECTION.x + 0 * FLOW_DIRECTION.y;
            FLOW_DIRECTION = tmp;
            return true;
        }
	}
    return false;
}

void InnerInfluence::spawnAt(GridCell* c) {
    grid_->buildAt(c->getCol(), c->getRow(), grid_->SIMULATED_STATE, InteractiveGrid::BuildMode::Additive);
}