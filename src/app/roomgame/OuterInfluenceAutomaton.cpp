#include "OuterInfluenceAutomaton.h"

OuterInfluenceAutomaton::OuterInfluenceAutomaton(AutomatonGrid* grid, double transition_time) :
	GPUCellularAutomaton(grid, transition_time),
	movedir_(1,0),
	birth_thd_(0.4f),
	death_thd_(0.5f),
	room_nbors_ahead_thd_(0.2f),
	outer_infl_nbors_thd_(1),
	damage_per_cell_(5)
{

}

void OuterInfluenceAutomaton::init(viscom::GPUProgramManager mgr) {
	GPUCellularAutomaton::init(mgr);
	movedir_uniform_location_ = shader_->getUniformLocation("moveDirection");
	birth_thd_uloc_ = shader_->getUniformLocation("BIRTH_THRESHOLD");
	death_thd_uloc_ = shader_->getUniformLocation("DEATH_THRESHOLD");
	room_nbors_ahead_thd_uloc_ = shader_->getUniformLocation("ROOM_NBORS_AHEAD_THRESHOLD");
	outer_infl_nbors_thd_uloc_ = shader_->getUniformLocation("OUTER_INFL_NBORS_THRESHOLD");
	damage_per_cell_uloc_ = shader_->getUniformLocation("DAMAGE_PER_CELL");
}


void OuterInfluenceAutomaton::transition(double time) {
	if (is_initialized_) {
		glUseProgram(shader_->getProgramId());
		glUniform2i(movedir_uniform_location_, movedir_.x, movedir_.y);
		glUniform1f(birth_thd_uloc_, birth_thd_);
		glUniform1f(death_thd_uloc_, death_thd_);
		glUniform1f(room_nbors_ahead_thd_uloc_, room_nbors_ahead_thd_);
		glUniform1i(outer_infl_nbors_thd_uloc_, outer_infl_nbors_thd_);
		glUniform1i(damage_per_cell_uloc_, damage_per_cell_);
	}
	GPUCellularAutomaton::transition(time);
}

void OuterInfluenceAutomaton::setMoveDir(int x, int y) {
	movedir_.x = x;
	movedir_.y = y;
}

void OuterInfluenceAutomaton::setBirthThreshold(GLfloat v) {
	birth_thd_ = v;
}

void OuterInfluenceAutomaton::setDeathThreshold(GLfloat v) {
	death_thd_ = v;
}

void OuterInfluenceAutomaton::setCollisionThreshold(GLfloat v) {
	room_nbors_ahead_thd_ = v;
}

void OuterInfluenceAutomaton::setOuterInfluenceNeighborThreshold(GLint v) {
	outer_infl_nbors_thd_ = v;
}

void OuterInfluenceAutomaton::setDamagePerCell(GLint v) {
	damage_per_cell_ = v;
}