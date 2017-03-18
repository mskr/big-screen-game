#include "GPUCellularAutomaton.h"

GPUCellularAutomaton::GPUCellularAutomaton(AutomatonGrid* grid) {
	grid_ = grid;
	grid_->setCellularAutomaton(this);
	pixel_size_ = glm::vec2(1.0f / float(grid->getNumColumns()), 1.0f / float(grid->getNumRows()));
	transition_time_ = 0.5;
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
	texture_pair_[0].sized_format = texture_pair_[1].sized_format = GL_RG32I;
	texture_pair_[0].format = texture_pair_[1].format = GL_RG_INTEGER;
	texture_pair_[0].datatype = texture_pair_[1].datatype = GL_INT;
	framebuffer_pair_[0] = new GPUBuffer(cols, rows, { &texture_pair_[0] });
	framebuffer_pair_[1] = new GPUBuffer(cols, rows, { &texture_pair_[1] });
	// Temporary client buffer to transfer pixels from and to
	size_t bytes = cols * rows * 2 * sizeof(GLint);
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
		for (unsigned int y = 0; y < rows*2 - 1; y+=2) {
			GridCell* c = grid_->getCellAt(y/2, x);
			tmp_client_buffer_[x*2*rows + y] = c->getBuildState();
			tmp_client_buffer_[x*2*rows + y + 1] = c->getHealthPoints();
		}
	}
	glBindTexture(GL_TEXTURE_2D, texture_pair_[pair_index].id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)cols, (GLsizei)rows, GL_RG_INTEGER, GL_INT, tmp_client_buffer_);
}

void GPUCellularAutomaton::copyFromTextureToGrid(int pair_index) {
	size_t rows = grid_->getNumColumns();
	size_t cols = grid_->getNumRows();
	glBindTexture(GL_TEXTURE_2D, texture_pair_[pair_index].id);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RG_INTEGER, GL_INT, tmp_client_buffer_);
	GLenum err;
	while (err = glGetError() != GL_NO_ERROR) printf("GL error %x in copyFromTextureToGrid\n", err);
	for (unsigned int x = 0; x < cols; x++) {
		for (unsigned int y = 0; y < rows*2 - 1; y+=2) {
			GLint state = tmp_client_buffer_[x*2*rows + y];
			GLint hp = tmp_client_buffer_[x*2*rows + y + 1];
			GridCell* c = grid_->getCellAt(y/2, x);
			grid_->buildAt(y/2, x, (GridCell::BuildState)state);
			c->setHealthPoints(hp);
		}
	}
}

void GPUCellularAutomaton::transition(double time) {
	// Test if simulation can begin
	if (!is_initialized_) return;
	// Test if it is time for the next generation
	if ((time - last_time_) >= transition_time_) {
		last_time_ = time;
	}
	else {
		return; //TODO interpolate
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