#include "GPUCellularAutomaton.h"

GPUCellularAutomaton::GPUCellularAutomaton(AutomatonGrid* grid, double transition_time) {
	grid_ = grid;
	grid_->setCellularAutomaton(this);
	pixel_size_ = glm::vec2(1.0f / float(grid->getNumColumns()), 1.0f / float(grid->getNumRows()));
	transition_time_ = transition_time;
	last_time_ = 0.0;
	is_initialized_ = false;
	current_read_index_ = 0;
}

void GPUCellularAutomaton::cleanup() {
	if (is_initialized_) {
		glDeleteTextures(1, &texture_pair_[0].id);
		glDeleteTextures(1, &texture_pair_[1].id);
		delete framebuffer_pair_[0];
		delete framebuffer_pair_[1];
		free(tmp_client_buffer_);
	}
}

void GPUCellularAutomaton::init(viscom::GPUProgramManager mgr) {
	if (is_initialized_) return;
	// Shader
	shader_ = mgr.GetResource("cellularAutomaton",
		std::initializer_list<std::string>{ "cellularAutomaton.vert", "cellularAutomaton.frag" });
	pixel_size_uniform_location_ = shader_->getUniformLocation("pxsize");
	texture_uniform_location_ = shader_->getUniformLocation("inputGrid");
	// Two framebuffers with textures
	GLuint cols = (GLuint)grid_->getNumColumns();
	GLuint rows = (GLuint)grid_->getNumRows();
	texture_pair_[0].attachmentType = texture_pair_[1].attachmentType = GL_COLOR_ATTACHMENT0;
	texture_pair_[0].sized_format = texture_pair_[1].sized_format = GL_RGBA32I;
	texture_pair_[0].format = texture_pair_[1].format = GL_RGBA_INTEGER;
	texture_pair_[0].datatype = texture_pair_[1].datatype = GL_INT;
	framebuffer_pair_[0] = new GPUBuffer(cols, rows, { &texture_pair_[0] });
	framebuffer_pair_[1] = new GPUBuffer(cols, rows, { &texture_pair_[1] });
	// Temporary client buffer to transfer pixels from and to
	size_t bytes = cols * rows * NUM_TEXTURE_CHANNELS * sizeof(GLint);
	tmp_client_buffer_ = (GLint*)malloc(bytes);
	if (!tmp_client_buffer_) throw std::runtime_error("");
	// Get initial state of grid
	copyFromGridToTexture(0);
	// Screen filling quad
	glGenVertexArrays(1, &vao_);
	glBindVertexArray(vao_);
	GLfloat quad[] = {
		// (x, y)      // (u, v)
		-1.0f,  1.0f,  0.0f, 1.0f, // top left
		1.0f, -1.0f,  1.0f, 0.0f, // bottom right
		-1.0f, -1.0f,  0.0f, 0.0f, // bottom left
		-1.0f,  1.0f,  0.0f, 1.0f, // top left
		1.0f,  1.0f,  1.0f, 1.0f, // top right
		1.0f, -1.0f,  1.0f, 0.0f // bottom right
	};
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
	is_initialized_ = true;
}

void GPUCellularAutomaton::copyFromGridToTexture(int pair_index) {
	size_t rows = grid_->getNumColumns();
	size_t cols = grid_->getNumRows();
	for (unsigned int x = 0; x < cols; x++) {
		for (unsigned int y = 0; y < rows*NUM_TEXTURE_CHANNELS - 1; y+=NUM_TEXTURE_CHANNELS) {
			GridCell* c = grid_->getCellAt(y/NUM_TEXTURE_CHANNELS, x);
			tmp_client_buffer_[x*NUM_TEXTURE_CHANNELS*rows + y] = c->getBuildState();
			tmp_client_buffer_[x*NUM_TEXTURE_CHANNELS*rows + y + 1] = c->getHealthPoints();
			tmp_client_buffer_[x*NUM_TEXTURE_CHANNELS*rows + y + 2] = 0;
			tmp_client_buffer_[x*NUM_TEXTURE_CHANNELS*rows + y + 3] = 0;
		}
	}
	glBindTexture(GL_TEXTURE_2D, texture_pair_[pair_index].id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)cols, (GLsizei)rows, GL_RGBA_INTEGER, GL_INT, tmp_client_buffer_);
}

void GPUCellularAutomaton::copyFromTextureToGrid(int pair_index) {
	size_t rows = grid_->getNumColumns();
	size_t cols = grid_->getNumRows();
	glBindTexture(GL_TEXTURE_2D, texture_pair_[pair_index].id);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA_INTEGER, GL_INT, tmp_client_buffer_);
	GLenum err;
	while (err = glGetError() != GL_NO_ERROR) printf("GL error %x in copyFromTextureToGrid\n", err);
	for (unsigned int x = 0; x < cols; x++) {
		for (unsigned int y = 0; y < rows*NUM_TEXTURE_CHANNELS - 1; y+=NUM_TEXTURE_CHANNELS) {
			GLint state = tmp_client_buffer_[x*NUM_TEXTURE_CHANNELS*rows + y];
			GLint hp = tmp_client_buffer_[x*NUM_TEXTURE_CHANNELS*rows + y + 1];
			GLint packedNbors_N_NE_E_SE = tmp_client_buffer_[x*NUM_TEXTURE_CHANNELS*rows + y + 2];
			GLint packedNbors_S_SW_W_NW = tmp_client_buffer_[x*NUM_TEXTURE_CHANNELS*rows + y + 3];
			grid_->updateGridOnly(y/NUM_TEXTURE_CHANNELS, x, (GridCell::BuildState)state, hp,
				glm::ivec2(packedNbors_N_NE_E_SE, packedNbors_S_SW_W_NW));
		}
	}
	// this updates the after-transition neighbors in shader for interpolation
	grid_->forEachCell([](GridCell* c) {
		if (c->getBuildState() == GridCell::BuildState::OUTER_INFLUENCE) {
			auto r = c->getMeshInstance();
			int neighborhood[8];
			neighborhood[0] = c->getNorthNeighbor()->getBuildState();
			neighborhood[1] = c->getNorthNeighbor()->getEastNeighbor()->getBuildState();
			neighborhood[2] = c->getEastNeighbor()->getBuildState();
			neighborhood[3] = c->getSouthNeighbor()->getEastNeighbor()->getBuildState();
			neighborhood[4] = c->getSouthNeighbor()->getBuildState();
			neighborhood[5] = c->getSouthNeighbor()->getWestNeighbor()->getBuildState();
			neighborhood[6] = c->getWestNeighbor()->getBuildState();
			neighborhood[7] = c->getNorthNeighbor()->getWestNeighbor()->getBuildState();
			int packedNbors_N_NE_E_SE = 0;
			for (int i = 0; i < 4; i++) {
				packedNbors_N_NE_E_SE = packedNbors_N_NE_E_SE | (neighborhood[i] << (i * 8));
			}
			int packedNbors_S_SW_W_NW = 0;
			for (int i = 4; i < 8; i++) {
				packedNbors_S_SW_W_NW = packedNbors_S_SW_W_NW | (neighborhood[i] << ((i - 4) * 8));
			}
			RoomSegmentMesh::Instance::updateNeighborBuildStatesPacked(r.buffer_->id_, r.offset_instances_,
				glm::ivec2(packedNbors_N_NE_E_SE, packedNbors_S_SW_W_NW));
		}
	});
}

void GPUCellularAutomaton::updateCell(size_t x, size_t y, GLint buildState, GLint hp) {
	GLint data[4] = { buildState, hp, 0, 0 };
	glBindTexture(GL_TEXTURE_2D, texture_pair_[current_read_index_].id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, (GLint)x, (GLint)y, 1, 1, GL_RGBA_INTEGER, GL_INT, data);
}

void GPUCellularAutomaton::transition(double time) {
	// Test if simulation can begin
	if (!is_initialized_) return;
	// Test if it is time for the next generation
	double delta = (time - last_time_);
	if (delta >= transition_time_) {
		last_time_ = time;
	}
	else {
		grid_->setUniformAutomatonTimeDelta((GLfloat)(delta/transition_time_)); // enables interpolation in shader
		return;
	}
	int current_write_index = (current_read_index_ == 0) ? 1 : 0;
	// Do transition on gpu
	framebuffer_pair_[current_write_index]->bind();
	glViewport(0, 0, (GLsizei)grid_->getNumColumns(), (GLsizei)grid_->getNumRows());
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(shader_->getProgramId());
	glBindVertexArray(vao_);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_pair_[current_read_index_].id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // repeat makes a torus-shaped playing field
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glUniform1i(texture_uniform_location_, 0);
	glUniform2f(pixel_size_uniform_location_, pixel_size_.x, pixel_size_.y);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	glEnable(GL_DEPTH_TEST);
	// Update grid
	copyFromTextureToGrid(current_write_index);
	// Swap buffers
	current_read_index_ = current_write_index;
}

void GPUCellularAutomaton::setTransitionTime(double t) {
	transition_time_ = t;
}