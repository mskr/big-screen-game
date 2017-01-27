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
}

InteractiveGrid::~InteractiveGrid() {
	interactions_.clear();
	rooms_.clear();
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

GridCell* InteractiveGrid::getCellAt(size_t col, size_t row) {
	if (col >= cells_.size() || row >= cells_[0].size()) return 0;
	return &cells_[col][row];
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

// TODO What happens with concurrent input?

void InteractiveGrid::onTouch(int touchID) {
	//GLint viewport[4]; // [x,y,width,height]
	//glGetIntegerv(GL_VIEWPORT, viewport);
	// Create interaction
	glm::vec2 touchPositionNDC =
		glm::vec2(last_mouse_position_.x, 1.0 - last_mouse_position_.y)
		* glm::vec2(2.0, 2.0) - glm::vec2(1.0, 1.0);
	GridCell* maybeCell = getCellAt(touchPositionNDC);
	if (!maybeCell) return;
	if (maybeCell->getBuildState() != GridCell::BuildState::EMPTY) return;
	Room* room = new Room(maybeCell, maybeCell, this);
	interactions_.push_back(
		new GridInteraction(touchID, last_mouse_position_, maybeCell, room));
}

void InteractiveGrid::onRelease(int touchID) {
	// Find and remove interaction
	for (GridInteraction* interac : interactions_) {
		if (interac->getTouchID() == touchID) {
			interac->update(last_mouse_position_);
			if (touchID == -1) {
				Room* room = interac->getRoom();
				if (room->isValid()) rooms_.push_back(room);
				else delete room;
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
				if (!maybeCell) {
					Room* room = interac->getRoom();
					if (room->isValid()) rooms_.push_back(room);
					else delete room;
					interactions_.remove(interac);
					return; // cursor was outside grid
				}
				if (interac->getLastCell() == maybeCell) return; // cursor was still inside last cell
				bool collision = (maybeCell->getBuildState() != GridCell::BuildState::EMPTY);
				resizeRoom(interac->getRoom(), interac->getStartCell(), interac->getLastCell(), maybeCell, collision);
				interac->update(maybeCell);
				break;
			}
		}
	}
}

void InteractiveGrid::updateBuildStateAt(size_t col, size_t row, GridCell::BuildState buildState) {
	GridCell* maybeCell = getCellAt(col, row);
	if (!maybeCell) return;
	maybeCell->updateBuildState(vbo_, buildState);
}

void InteractiveGrid::resizeRoom(Room* room, GridCell* startCell, GridCell* lastCell, GridCell* currentCell, bool collision) {
	//TODO Collision cases that are not handled yet:
	// a) spanning a invalid room into another room
	// b) growing a room so that the corner that is not the current cell hits another room
	// c) wrapping other rooms by dragging current cell around them
	if (startCell == lastCell || !room->isValid()) {
		// Handle degenerated rooms
		room->clear();
		room->spanFromTo(startCell, currentCell);
		return;
	}
	// Compute size delta and update room (room updates GPU data)
	size_t colDist = lastCell->getColDistanceTo(currentCell);
	size_t rowDist = lastCell->getRowDistanceTo(currentCell);
	// Horizontal
	if (startCell->isWestOf(lastCell)) {
		if (lastCell->isWestOf(currentCell) && !collision)
			room->growToEast(colDist);
		else if (lastCell->isEastOf(currentCell))
			room->shrinkToWest(colDist);
	}
	else if (startCell->isEastOf(lastCell)) {
		if (lastCell->isEastOf(currentCell) && !collision)
			room->growToWest(colDist);
		else if (lastCell->isWestOf(currentCell))
			room->shrinkToEast(colDist);
	}
	// Vertical
	if (startCell->isNorthOf(lastCell)) {
		if (lastCell->isNorthOf(currentCell) && !collision)
			room->growToSouth(rowDist);
		else if (lastCell->isSouthOf(currentCell))
			room->shrinkToNorth(rowDist);
	}
	else if (startCell->isSouthOf(lastCell)) {
		if (lastCell->isSouthOf(currentCell) && !collision)
			room->growToNorth(rowDist);
		else if (lastCell->isNorthOf(currentCell))
			room->shrinkToSouth(rowDist);
	}
}