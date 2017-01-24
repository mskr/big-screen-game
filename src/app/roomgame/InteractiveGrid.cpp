#include "InteractiveGrid.h"

InteractiveGrid::InteractiveGrid(int columns, int rows, float height) {
	height_units_ = height;
	cell_size_ = height_units_ / float(rows);
	for (int x = 0; x < columns; x++) {
		std::vector<GridCell> column;
		for (int y = 0; y < rows; y++) {
			GridCell current(-1.0f + x * cell_size_, -1.0f + y * cell_size_, x, y);
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
	model_matrix_ = glm::translate(glm::mat4(1), glm::vec3(-0.5f, 0.3f, 0.0f));
	num_vertices_ = 0;
	last_sgctMVP_ = glm::mat4(1);
	last_room_start_cell_ = 0;
	last_room_end_cell_ = 0;
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

void InteractiveGrid::forEachCellInRange(GridCell* leftLower, GridCell* rightUpper, std::function<void(GridCell*)> callback) {
	if (leftLower->getCol() > rightUpper->getCol() || leftLower->getRow() > rightUpper->getRow())
		return;
	for (size_t i = leftLower->getCol(); i <= rightUpper->getCol(); i++) {
		for (size_t j = leftLower->getRow(); j <= rightUpper->getRow(); j++) {
			callback(&cells_[i][j]);
		}
	}
}

bool InteractiveGrid::isInsideGrid(glm::vec2 positionNDC) {
	GridCell& leftUpperCell = cells_[0][cells_[0].size() - 1];
	glm::vec2 posLeftUpperNDC = getNDC(leftUpperCell.getPosition());
	if (positionNDC.x < posLeftUpperNDC.x || positionNDC.y > posLeftUpperNDC.y)
		return false;
	GridCell& rightLowerCell = cells_[cells_.size() - 1][0];
	glm::vec2 posRightLowerNDC = getNDC(glm::vec2(
		rightLowerCell.getXPosition() + cell_size_, rightLowerCell.getYPosition() - cell_size_ ));
	if (positionNDC.x > posRightLowerNDC.x || positionNDC.y < posRightLowerNDC.y)
		return false;
	return true;
}

bool InteractiveGrid::isInsideCell(glm::vec2 positionNDC, GridCell* cell) {
	glm::vec2 cellLeftUpperNDC = getNDC(cell->getPosition());
	glm::vec2 cellRightLowerNDC = getNDC(glm::vec2(
		cell->getXPosition() + cell_size_, cell->getYPosition() - cell_size_ ));
	if (positionNDC.x < cellLeftUpperNDC.x || positionNDC.y > cellLeftUpperNDC.y)
		return false;
	else if (positionNDC.x > cellRightLowerNDC.x || positionNDC.y < cellRightLowerNDC.y)
		return false;
	return true;
}

glm::vec2 InteractiveGrid::getNDC(glm::vec2 position) {
	glm::vec4 pos(position, 0.0f, 1.0f);
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
	while (iRightLower - iLeftUpper > 2 || jLeftUpper - jRightLower > 2) {
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
	for (size_t i = iLeftUpper; i <= iRightLower; i++) {
		for (size_t j = jRightLower; j <= jLeftUpper; j++) {
			if (isInsideCell(positionNDC, &cells_[i][j]))
				return &cells_[i][j];
		}
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
}

void InteractiveGrid::render(glm::mat4 sgctMVP) {
	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glUseProgram(shader_->getProgramId());
	glUniformMatrix4fv(mvp_uniform_location_, 1, GL_FALSE, glm::value_ptr(sgctMVP * model_matrix_));
	glDrawArrays(GL_POINTS, 0, num_vertices_);
	last_sgctMVP_ = sgctMVP;
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
			interac->update(last_mouse_position_);
			if (touchID == -1) {
				glm::vec2 touchPositionNDC =
					glm::vec2(last_mouse_position_.x, 1.0 - last_mouse_position_.y)
					* glm::vec2(2.0, 2.0) - glm::vec2(1.0, 1.0);
				GridCell* maybeCell = getCellAt(touchPositionNDC);
				if (maybeCell)
					addRoom(interac->getStartCell(), maybeCell, true);
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
			interac->update(last_mouse_position_);
			if (touchID == -1) {
				glm::vec2 touchPositionNDC =
					glm::vec2(last_mouse_position_.x, 1.0 - last_mouse_position_.y)
					* glm::vec2(2.0, 2.0) - glm::vec2(1.0, 1.0);
				GridCell* maybeCell = getCellAt(touchPositionNDC);
				if (!maybeCell) return;
				if (interac->getStartCell() == maybeCell) return;
				if (interac->getLastCell() == maybeCell) return;
				interac->update(maybeCell);
				addRoom(interac->getStartCell(), maybeCell, false);
				break;
			}
		}
	}
}

void InteractiveGrid::addRoom(GridCell* startCell, GridCell* endCell, bool isFinished) {
	GridCell* leftLowerCorner = 0;
	GridCell* rightUpperCorner = 0;
	GridCell* leftUpperCorner = 0;
	GridCell* rightLowerCorner = 0;
	// Find corners
	if (startCell->getCol() < endCell->getCol()) {
		if (startCell->getRow() < endCell->getRow()) {
			leftLowerCorner = startCell;
			rightUpperCorner = endCell;
			leftUpperCorner = &cells_[startCell->getCol()][endCell->getRow()];
			rightLowerCorner = &cells_[endCell->getCol()][startCell->getRow()];
		}
		else {
			leftUpperCorner = startCell;
			rightLowerCorner = endCell;
			leftLowerCorner = &cells_[startCell->getCol()][endCell->getRow()];
			rightUpperCorner = &cells_[endCell->getCol()][startCell->getRow()];
		}
	}
	else {
		if (startCell->getRow() < endCell->getRow()) {
			rightLowerCorner = startCell;
			leftUpperCorner = endCell;
			rightUpperCorner = &cells_[startCell->getCol()][endCell->getRow()];
			leftLowerCorner = &cells_[endCell->getCol()][startCell->getRow()];
		}
		else {
			rightUpperCorner = startCell;
			leftLowerCorner = endCell;
			rightLowerCorner = &cells_[startCell->getCol()][endCell->getRow()];
			leftUpperCorner = &cells_[endCell->getCol()][startCell->getRow()];
		}
	}
	// Test room min size
	if (rightUpperCorner->getCol() - leftLowerCorner->getCol() < ROOM_MIN_SIZE_ ||
			rightUpperCorner->getRow() - leftLowerCorner->getRow() < ROOM_MIN_SIZE_) {
		forEachCellInRange(leftLowerCorner, rightUpperCorner, [&](GridCell* cell) {
			cell->updateBuildState(vbo_, GridCell::BuildState::INVALID);
		});
		return;
	}
	// Set build states
	leftLowerCorner->updateBuildState(vbo_, GridCell::BuildState::LEFT_LOWER_CORNER);
	rightUpperCorner->updateBuildState(vbo_, GridCell::BuildState::RIGHT_UPPER_CORNER);
	leftUpperCorner->updateBuildState(vbo_, GridCell::BuildState::LEFT_UPPER_CORNER);
	rightLowerCorner->updateBuildState(vbo_, GridCell::BuildState::RIGHT_LOWER_CORNER);
	forEachCellInRange(leftUpperCorner->getEastNeighbor(), rightUpperCorner->getWestNeighbor(), [&](GridCell* cell) {
		cell->updateBuildState(vbo_, GridCell::BuildState::WALL_TOP);
	});
	forEachCellInRange(leftLowerCorner->getEastNeighbor(), rightLowerCorner->getWestNeighbor(), [&](GridCell* cell) {
		cell->updateBuildState(vbo_, GridCell::BuildState::WALL_BOTTOM);
	});
	forEachCellInRange(leftLowerCorner->getNorthNeighbor(), leftUpperCorner->getSouthNeighbor(), [&](GridCell* cell) {
		cell->updateBuildState(vbo_, GridCell::BuildState::WALL_LEFT);
	});
	forEachCellInRange(rightLowerCorner->getNorthNeighbor(), rightUpperCorner->getSouthNeighbor(), [&](GridCell* cell) {
		cell->updateBuildState(vbo_, GridCell::BuildState::WALL_RIGHT);
	});
	GridCell* insideLeftLower = leftLowerCorner->getEastNeighbor()->getNorthNeighbor();
	GridCell* insideRightUpper = rightUpperCorner->getWestNeighbor()->getSouthNeighbor();
	forEachCellInRange(insideLeftLower, insideRightUpper, [&](GridCell* cell) {
		cell->updateBuildState(vbo_, GridCell::BuildState::INSIDE_ROOM);
	});
}