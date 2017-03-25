#ifndef GPU_CELLULAR_AUTOMATON_H
#define GPU_CELLULAR_AUTOMATON_H

#include "AutomatonGrid.h"
#include "GPUBuffer.h"

class GPUCellularAutomaton {
protected:
	AutomatonGrid* grid_;
	GPUBuffer* framebuffer_pair_[2];
	GPUBuffer::Tex texture_pair_[2];
	int current_read_index_;
	GLint* tmp_client_buffer_;
	GLuint vao_;
	std::shared_ptr<viscom::GPUProgram> shader_;
	GLint pixel_size_uniform_location_;
	GLint texture_uniform_location_;
	glm::vec2 pixel_size_;
	double transition_time_;
	double last_time_;
	bool is_initialized_;
	// Helper
	void copyFromGridToTexture(int pair_index);
	void copyFromTextureToGrid(int pair_index);
public:
	GPUCellularAutomaton(AutomatonGrid* grid, double transition_time);
	void updateCell(size_t x, size_t y, GLint state, GLint hp);
	virtual void init(viscom::GPUProgramManager mgr);
	virtual void transition(double time);
	void cleanup();
	//Setter
	void setTransitionTime(double);
};

class OuterInfluenceAutomaton : public GPUCellularAutomaton {
	GLint movedir_uniform_location_;
	glm::ivec2 movedir_;
	GLint birth_thd_uloc_;
	GLfloat birth_thd_;
	GLint death_thd_uloc_;
	GLfloat death_thd_;
	GLint room_nbors_ahead_thd_uloc_; // collision threshold
	GLfloat room_nbors_ahead_thd_;
	GLint outer_infl_nbors_thd_uloc_;
	GLint outer_infl_nbors_thd_;
	GLint damage_per_cell_uloc_;
	GLint damage_per_cell_;
public:
	OuterInfluenceAutomaton(AutomatonGrid* grid, double transition_time) :
		GPUCellularAutomaton(grid, transition_time),
		movedir_(1,0),
		birth_thd_(0.4f),
		death_thd_(0.5f),
		room_nbors_ahead_thd_(0.2f),
		outer_infl_nbors_thd_(1),
		damage_per_cell_(5) {}
	void init(viscom::GPUProgramManager mgr) {
		GPUCellularAutomaton::init(mgr);
		movedir_uniform_location_ = shader_->getUniformLocation("moveDirection");
		birth_thd_uloc_ = shader_->getUniformLocation("BIRTH_THRESHOLD");
		death_thd_uloc_ = shader_->getUniformLocation("DEATH_THRESHOLD");
		room_nbors_ahead_thd_uloc_ = shader_->getUniformLocation("ROOM_NBORS_AHEAD_THRESHOLD");
		outer_infl_nbors_thd_uloc_ = shader_->getUniformLocation("OUTER_INFL_NBORS_THRESHOLD");
		damage_per_cell_uloc_ = shader_->getUniformLocation("DAMAGE_PER_CELL");
	}
	void setMoveDir(int x, int y) {
		movedir_.x = x;
		movedir_.y = y;
	}
	void setBirthThreshold(GLfloat v) {
		birth_thd_ = v;
	}
	void setDeathThreshold(GLfloat v) {
		death_thd_ = v;
	}
	void setCollisionThreshold(GLfloat v) {
		room_nbors_ahead_thd_ = v;
	}
	void setOuterInfluenceNeighborThreshold(GLint v) {
		outer_infl_nbors_thd_ = v;
	}
	void setDamagePerCell(GLint v) {
		damage_per_cell_ = v;
	}
	void transition(double time) {
		if (!is_initialized_) return;
		glUseProgram(shader_->getProgramId());
		glUniform2i(movedir_uniform_location_, movedir_.x, movedir_.y);
		glUniform1f(birth_thd_uloc_, birth_thd_);
		glUniform1f(death_thd_uloc_, death_thd_);
		glUniform1f(room_nbors_ahead_thd_uloc_, room_nbors_ahead_thd_);
		glUniform1i(outer_infl_nbors_thd_uloc_, outer_infl_nbors_thd_);
		glUniform1i(damage_per_cell_uloc_, damage_per_cell_);
		GPUCellularAutomaton::transition(time);
	}
};

#endif