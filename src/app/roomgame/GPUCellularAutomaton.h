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
	GLubyte* tmp_client_buffer_;
	GLuint vao_;
	std::shared_ptr<viscom::GPUProgram> shader_;
	GLint pixel_size_uniform_location_;
	GLint texture_uniform_location_;
	glm::vec2 pixel_size_;
	double transition_time_;
	double last_time_;
	double delta_time_;
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
	//Getter
	GLfloat getTimeDeltaNormalized();
	GLuint getLatestTexture();
	GLuint getPreviousTexture();
	bool isInitialized();
};

#endif