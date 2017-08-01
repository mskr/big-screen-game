#include "InnerInfluence.h"

InnerInfluence::InnerInfluence(AutomatonGrid* grid, double transition_time) :
    SynchronizedAutomaton(grid, transition_time),
	movedir_(1,0),
	birth_thd_(0.4f),
	death_thd_(0.5f),
	room_nbors_ahead_thd_(0.2f),
	outer_infl_nbors_thd_(1),
	damage_per_cell_(5)
{

}

void InnerInfluence::init(viscom::GPUProgramManager mgr) {
    SynchronizedAutomaton::init(mgr);
	movedir_uniform_location_ = shader_->getUniformLocation("moveDirection");
	birth_thd_uloc_ = shader_->getUniformLocation("BIRTH_THRESHOLD");
	death_thd_uloc_ = shader_->getUniformLocation("DEATH_THRESHOLD");
	room_nbors_ahead_thd_uloc_ = shader_->getUniformLocation("ROOM_NBORS_AHEAD_THRESHOLD");
	outer_infl_nbors_thd_uloc_ = shader_->getUniformLocation("OUTER_INFL_NBORS_THRESHOLD");
	damage_per_cell_uloc_ = shader_->getUniformLocation("DAMAGE_PER_CELL");
}


void InnerInfluence::transition(double time) {
	if (GPUCellularAutomaton::isInitialized()) {
		glUseProgram(shader_->getProgramId());
		glUniform2i(movedir_uniform_location_, movedir_.x, movedir_.y);
		glUniform1f(birth_thd_uloc_, birth_thd_);
		glUniform1f(death_thd_uloc_, death_thd_);
		glUniform1f(room_nbors_ahead_thd_uloc_, room_nbors_ahead_thd_);
		glUniform1i(outer_infl_nbors_thd_uloc_, outer_infl_nbors_thd_);
		glUniform1i(damage_per_cell_uloc_, damage_per_cell_);
        SynchronizedAutomaton::transition(time);
	}
}

void InnerInfluence::spawnAt(GridCell* c) {
    grid_->buildAt(c->getCol(), c->getRow(), grid_->SIMULATED_STATE);
}

void InnerInfluence::setMoveDir(int x, int y) {
	movedir_.x = x;
	movedir_.y = y;
}

void InnerInfluence::setBirthThreshold(GLfloat v) {
	birth_thd_ = v;
}

void InnerInfluence::setDeathThreshold(GLfloat v) {
	death_thd_ = v;
}

void InnerInfluence::setCollisionThreshold(GLfloat v) {
	room_nbors_ahead_thd_ = v;
}

void InnerInfluence::setOuterInfluenceNeighborThreshold(GLint v) {
	outer_infl_nbors_thd_ = v;
}

void InnerInfluence::setDamagePerCell(GLint v) {
	damage_per_cell_ = v;
}