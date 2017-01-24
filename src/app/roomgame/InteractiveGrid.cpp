#include "InteractiveGrid.h"

InteractiveGrid::InteractiveGrid(int columns, int rows) {
	cell_size_ = height_units_ / float(rows);
	z_distance_ = 0.0f;
	for (int x = 0; x < columns; x++) {
		std::vector<GridCell> column;
		for (int y = 0; y < rows; y++) {
			GridCell current(-1.0f + x * cell_size_, -1.0f + y * cell_size_);
			column.push_back(current);
		}
		cells_.push_back(column);
	}
	for (int x = 0; x < columns; x++) {
		for (int y = 0; y < rows; y++) {
			cells_[x][y].setNorthNeighbor((y == rows - 1) ? 0 : &cells_[x][y + 1]);
			cells_[x][y].setEastNeighbor((x == columns - 1) ? 0 : &cells_[x + 1][y]);
			cells_[x][y].setSouthNeighbor((y == 0) ? 0 : &cells_[x][y - 1]);
			cells_[x][y].setWestNeighbor((x == 0) ? 0 : &cells_[x - 1][y]);
		}
	}
	mvp_uniform_location_ = -1;
	z_distance_uniform_location_ = -1;
	model_matrix_ = glm::translate(glm::mat4(1), glm::vec3(-0.5f, 1.0f, 0.0f));
	num_vertices_ = 0;
	last_sgctMVP_ = glm::mat4(1);
}

InteractiveGrid::~InteractiveGrid() {
	interactions_.clear();
}

void InteractiveGrid::forEachCell(std::function<void(GridCell*)> callback) {
	for (std::vector<GridCell> &row : cells_) {
		for (GridCell &cell : row) {
			callback(&cell);
		}
	}
}

void InteractiveGrid::forEachCell(std::function<void(GridCell*,bool*)> callback) {
	bool found = false;
	for (std::vector<GridCell> &row : cells_) {
		for (GridCell &cell : row) {
			callback(&cell, &found);
			if (found) break;
		}
		if (found) break;
	}
}

bool InteractiveGrid::isInsideGrid(glm::vec2 positionNDC) {
	GridCell& leftUpperCell = cells_[0][cells_[0].size() - 1];
	glm::vec2 posLeftUpperNDC = getNDC(leftUpperCell.getPosition());
	if (positionNDC.x < posLeftUpperNDC.x || positionNDC.y > posLeftUpperNDC.y)
		return false;
	GridCell& rightLowerCell = cells_[cells_.size() - 1][0];
	glm::vec2 posRightLowerNDC = getNDC(rightLowerCell.getPosition() + cell_size_);
	if (positionNDC.x > posRightLowerNDC.x || positionNDC.y < posRightLowerNDC.y)
		return false;
	return true;
}

bool InteractiveGrid::isInsideCell(glm::vec2 positionNDC, GridCell* cell) {
	glm::vec2 cellLeftUpperNDC = getNDC(cell->getPosition());
	glm::vec2 cellRightLowerNDC = getNDC(cell->getPosition() + cell_size_);
	if (positionNDC.x < cellLeftUpperNDC.x || positionNDC.y > cellLeftUpperNDC.y)
		return false;
	else if (positionNDC.x > cellRightLowerNDC.x || positionNDC.y < cellRightLowerNDC.y)
		return false;
	return true;
}

glm::vec2 InteractiveGrid::getNDC(glm::vec2 position) {
	glm::vec4 pos(position, z_distance_, 1.0f);
	pos = last_sgctMVP_ * model_matrix_ * pos;
	return glm::vec2(pos.x, pos.y) / pos.w;
}

GridCell* InteractiveGrid::getCellAt(glm::vec2 positionNDC) {
	if (!isInsideGrid(positionNDC))
		return 0;
	size_t iLeftUpper = 0;
	size_t jLeftUpper = cells_[0].size() - 1;
	size_t iRightLower = cells_.size() - 1;
	size_t jRightLower = 0;
	while (!(iLeftUpper+1 == iRightLower && jLeftUpper == jRightLower+1)) {
		size_t iMiddle = iLeftUpper + (iRightLower - iLeftUpper) / 2;
		size_t jMiddle = jRightLower + (jLeftUpper - jRightLower) / 2;
		glm::vec2 cellNDC = getNDC(cells_[iMiddle][jMiddle].getPosition());
		if (positionNDC.x < cellNDC.x)
			iRightLower = iMiddle;
		else
			iLeftUpper = iMiddle;
		if (positionNDC.y < cellNDC.y)
			jLeftUpper = jMiddle;
		else
			jRightLower = jMiddle;
	}
	return &cells_[iLeftUpper][jLeftUpper];
}

void InteractiveGrid::uploadVertexData() {
	glGenVertexArrays(1, &vao_);
	glBindVertexArray(vao_);
	glGenBuffers(1, &vbo_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	size_t ncells = cells_.size() * cells_[0].size();
	size_t bytes_per_cell = GridCell::getVertexBytes();
	glBufferData(GL_ARRAY_BUFFER,
		ncells * bytes_per_cell,
		(void*)0,
		GL_STATIC_DRAW);
	GLintptr offset = 0;
	forEachCell([&](GridCell* cell) {
		cell->setVertexBufferOffset(offset);
		glBufferSubData(GL_ARRAY_BUFFER, offset,
			bytes_per_cell,
			cell->getVertexPointer());
		offset += bytes_per_cell;
	});
	GridCell::setVertexAttribPointer();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	num_vertices_ = (GLsizei)ncells;
}

void InteractiveGrid::loadShader(viscom::ApplicationNode* appNode) {
	glEnable(GL_POINT_SPRITE);
	glEnable(GL_PROGRAM_POINT_SIZE);
	shader_ = appNode->GetGPUProgramManager().GetResource("interactiveGrid",
		std::initializer_list<std::string>{ "interactiveGrid.vert", "interactiveGrid.frag" });
	mvp_uniform_location_ = shader_->getUniformLocation("MVP");
	z_distance_uniform_location_ = shader_->getUniformLocation("Z");
}

void InteractiveGrid::render(glm::mat4 sgctMVP) {
	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glUseProgram(shader_->getProgramId());
	glUniformMatrix4fv(mvp_uniform_location_, 1, GL_FALSE, glm::value_ptr(sgctMVP * model_matrix_));
	glUniform1f(z_distance_uniform_location_, z_distance_);
	glDrawArrays(GL_POINTS, 0, num_vertices_);
	last_sgctMVP_ = sgctMVP;
	//z_distance_ = -(float)(0.5f*glfwGetTime());
}

void InteractiveGrid::cleanup() {
	glDeleteBuffers(1, &vbo_);
	glDeleteVertexArrays(1, &vao_);
}

void InteractiveGrid::onTouch(int touchID) {
	//GLint viewport[4]; // [x,y,width,height]
	//glGetIntegerv(GL_VIEWPORT, viewport);
	glm::vec2 touchPositionNDC =
		glm::vec2(last_mouse_position_.x, 1.0 - last_mouse_position_.y)
		* glm::vec2(2.0, 2.0) - glm::vec2(1.0, 1.0);
	GridCell* maybeCell = getCellAt(touchPositionNDC);
	if (!maybeCell) return;
	interactions_.push_back(new GridInteraction(touchID, last_mouse_position_, maybeCell));
}

void InteractiveGrid::onRelease(int touchID) {
	for (GridInteraction* interac : interactions_) {
		if (interac->getTouchID() == touchID) {
			interac->onRelease(last_mouse_position_);
			if (touchID == -1) {
				glm::vec2 touchPositionNDC =
					glm::vec2(last_mouse_position_.x, 1.0 - last_mouse_position_.y)
					* glm::vec2(2.0, 2.0) - glm::vec2(1.0, 1.0);
				GridCell* maybeCell = getCellAt(touchPositionNDC);
				if (!maybeCell) return;
				addRoom(interac->getStartCell(), maybeCell);
				interactions_.remove(interac);
				break;
			}
			else {
				//TODO wait for possible end of discontinuaty
			}
		}
	}
}

void InteractiveGrid::onMouseMove(int touchID, double newx, double newy) {
	last_mouse_position_ = glm::dvec2(newx, newy);
	for (GridInteraction* interac : interactions_) {
		if (interac->getTouchID() == touchID) {
			interac->onRelease(last_mouse_position_);
			if (touchID == -1) {
				glm::vec2 touchPositionNDC =
					glm::vec2(last_mouse_position_.x, 1.0 - last_mouse_position_.y)
					* glm::vec2(2.0, 2.0) - glm::vec2(1.0, 1.0);
				GridCell* maybeCell = getCellAt(touchPositionNDC);
				if (!maybeCell) return;
				if (interac->getStartCell() == maybeCell) return;
				addRoom(interac->getStartCell(), maybeCell);
				break;
			}
		}
	}
}

void InteractiveGrid::addRoom(GridCell* startCell, GridCell* endCell) {
	//TODO Simplify!
	// 1) use vector indices of cells instead of float positions
	if (startCell->getXPosition() < endCell->getXPosition()) {
		if (startCell->getYPosition() < endCell->getYPosition()) {
			startCell->updateBuildState(vbo_, GridCell::BuildState::LEFT_LOWER_CORNER);
			endCell->updateBuildState(vbo_, GridCell::BuildState::RIGHT_UPPER_CORNER);
			GridCell* wall = startCell->getNorthNeighbor();
			if (!wall) return;
			while (wall->getYPosition() < endCell->getYPosition()) {
				wall->updateBuildState(vbo_, GridCell::BuildState::WALL);
				GridCell* inner = wall->getEastNeighbor();
				while (inner->getXPosition() < endCell->getXPosition()) {
					inner->updateBuildState(vbo_, GridCell::BuildState::INSIDE_ROOM);
					inner = inner->getEastNeighbor();
					if (!inner) break;
				}
				wall = wall->getNorthNeighbor();
				if (!wall) break;
			}
			if (wall) {
				wall->updateBuildState(vbo_, GridCell::BuildState::LEFT_UPPER_CORNER);
				wall = wall->getEastNeighbor();
				if (!wall) return;
				while (wall->getXPosition() < endCell->getXPosition()) {
					wall->updateBuildState(vbo_, GridCell::BuildState::WALL);
					wall = wall->getEastNeighbor();
					if (!wall) break;
				}
			}
		}
		else {
			startCell->updateBuildState(vbo_, GridCell::BuildState::LEFT_UPPER_CORNER);
			endCell->updateBuildState(vbo_, GridCell::BuildState::RIGHT_LOWER_CORNER);
			GridCell* wall = startCell->getSouthNeighbor();
			if (!wall) return;
			while (wall->getYPosition() > endCell->getYPosition()) {
				wall->updateBuildState(vbo_, GridCell::BuildState::WALL);
				GridCell* inner = wall->getEastNeighbor();
				while (inner->getXPosition() < endCell->getXPosition()) {
					inner->updateBuildState(vbo_, GridCell::BuildState::INSIDE_ROOM);
					inner = inner->getEastNeighbor();
					if (!inner) break;
				}
				wall = wall->getSouthNeighbor();
				if (!wall) break;
			}
			if (wall) {
				wall->updateBuildState(vbo_, GridCell::BuildState::LEFT_LOWER_CORNER);
				wall = wall->getEastNeighbor();
				if (!wall) return;
				while (wall->getXPosition() < endCell->getXPosition()) {
					wall->updateBuildState(vbo_, GridCell::BuildState::WALL);
					wall = wall->getEastNeighbor();
					if (!wall) break;
				}
			}
		}
		GridCell* wall = startCell->getEastNeighbor();
		if (!wall) return;
		while (wall->getXPosition() < endCell->getXPosition()) {
			wall->updateBuildState(vbo_, GridCell::BuildState::WALL);
			wall = wall->getEastNeighbor();
			if (!wall) break;
		}
		if (wall && startCell->getYPosition() < endCell->getYPosition()) {
			wall->updateBuildState(vbo_, GridCell::BuildState::RIGHT_LOWER_CORNER);
			wall = wall->getNorthNeighbor();
			while (wall->getYPosition() < endCell->getYPosition()) {
				wall->updateBuildState(vbo_, GridCell::BuildState::WALL);
				wall = wall->getNorthNeighbor();
				if (!wall) break;
			}
		}
		else if (wall) {
			wall->updateBuildState(vbo_, GridCell::BuildState::RIGHT_UPPER_CORNER);
			wall = wall->getSouthNeighbor();
			while (wall->getYPosition() > endCell->getYPosition()) {
				wall->updateBuildState(vbo_, GridCell::BuildState::WALL);
				wall = wall->getSouthNeighbor();
				if (!wall) break;
			}
		}
	}
	else {
		if (endCell->getYPosition() < startCell->getYPosition()) {
			endCell->updateBuildState(vbo_, GridCell::BuildState::LEFT_LOWER_CORNER);
			startCell->updateBuildState(vbo_, GridCell::BuildState::RIGHT_UPPER_CORNER);
			GridCell* wall = endCell->getNorthNeighbor();
			if (!wall) return;
			while (wall->getYPosition() < startCell->getYPosition()) {
				wall->updateBuildState(vbo_, GridCell::BuildState::WALL);
				GridCell* inner = wall->getEastNeighbor();
				while (inner->getXPosition() < startCell->getXPosition()) {
					inner->updateBuildState(vbo_, GridCell::BuildState::INSIDE_ROOM);
					inner = inner->getEastNeighbor();
					if (!inner) break;
				}
				wall = wall->getNorthNeighbor();
				if (!wall) break;
			}
			if (wall) {
				wall->updateBuildState(vbo_, GridCell::BuildState::LEFT_UPPER_CORNER);
				wall = wall->getEastNeighbor();
				if (!wall) return;
				while (wall->getXPosition() < startCell->getXPosition()) {
					wall->updateBuildState(vbo_, GridCell::BuildState::WALL);
					wall = wall->getEastNeighbor();
					if (!wall) break;
				}
			}
		}
		else {
			endCell->updateBuildState(vbo_, GridCell::BuildState::LEFT_UPPER_CORNER);
			startCell->updateBuildState(vbo_, GridCell::BuildState::RIGHT_LOWER_CORNER);
			GridCell* wall = endCell->getSouthNeighbor();
			if (!wall) return;
			while (wall->getYPosition() > startCell->getYPosition()) {
				wall->updateBuildState(vbo_, GridCell::BuildState::WALL);
				GridCell* inner = wall->getEastNeighbor();
				while (inner->getXPosition() < startCell->getXPosition()) {
					inner->updateBuildState(vbo_, GridCell::BuildState::INSIDE_ROOM);
					inner = inner->getEastNeighbor();
					if (!inner) break;
				}
				wall = wall->getSouthNeighbor();
				if (!wall) break;
			}
			if (wall) {
				wall->updateBuildState(vbo_, GridCell::BuildState::LEFT_LOWER_CORNER);
				wall = wall->getEastNeighbor();
				while (wall->getXPosition() < startCell->getXPosition()) {
					wall->updateBuildState(vbo_, GridCell::BuildState::WALL);
					wall = wall->getEastNeighbor();
					if (!wall) break;
				}
			}
		}
		GridCell* wall = endCell->getEastNeighbor();
		if (!wall) return;
		while (wall->getXPosition() < startCell->getXPosition()) {
			wall->updateBuildState(vbo_, GridCell::BuildState::WALL);
			wall = wall->getEastNeighbor();
			if (!wall) break;
		}
		if (wall && endCell->getYPosition() < startCell->getYPosition()) {
			wall->updateBuildState(vbo_, GridCell::BuildState::RIGHT_LOWER_CORNER);
			wall = wall->getNorthNeighbor();
			while (wall->getYPosition() < startCell->getYPosition()) {
				wall->updateBuildState(vbo_, GridCell::BuildState::WALL);
				wall = wall->getNorthNeighbor();
				if (!wall) break;
			}
		}
		else if (wall) {
			wall->updateBuildState(vbo_, GridCell::BuildState::RIGHT_UPPER_CORNER);
			wall = wall->getSouthNeighbor();
			while (wall->getYPosition() > startCell->getYPosition()) {
				wall->updateBuildState(vbo_, GridCell::BuildState::WALL);
				wall = wall->getSouthNeighbor();
				if (!wall) break;
			}
		}
	}
}