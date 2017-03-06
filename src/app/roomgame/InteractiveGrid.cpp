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
	model_matrix_ = glm::mat4(1);// glm::translate(glm::mat4(1), glm::vec3(-0.5f, 0.3f, 0.0f));
	num_vertices_ = 0;
	last_sgctMVP_ = glm::mat4(1);
	meshpool_ = 0;
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

void InteractiveGrid::forEachCellInRange(GridCell* leftLower, GridCell* rightUpper, std::function<void(GridCell*,bool*)> callback) {
	if (leftLower->getCol() > rightUpper->getCol() || leftLower->getRow() > rightUpper->getRow())
		return;
	bool found = false;
	for (size_t i = leftLower->getCol(); i <= rightUpper->getCol(); i++) {
		for (size_t j = leftLower->getRow(); j <= rightUpper->getRow(); j++) {
			callback(&cells_[i][j], &found);
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
	glDisable(GL_DEPTH_TEST);
	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glUseProgram(shader_->getProgramId());
	glUniformMatrix4fv(mvp_uniform_location_, 1, GL_FALSE, glm::value_ptr(sgctMVP * model_matrix_));
	glDrawArrays(GL_POINTS, 0, num_vertices_);
	glEnable(GL_DEPTH_TEST);
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
				if (room->isValid()) {
					// Finish room
					room->finish();
					rooms_.push_back(room);
				}
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
				Room::CollisionType collision = resizeRoomUntilCollision(interac->getRoom(), interac->getStartCell(), interac->getLastCell(), maybeCell);
				if (collision == Room::CollisionType::NONE) {
					interac->setLastCell(maybeCell);
				}
				else if (interac->getLastCollision() == Room::CollisionType::NONE) {
					interac->setLastCell(maybeCell);
				}
				else if (collision == Room::CollisionType::HORIZONTAL) {
					if (interac->getLastCollision() == Room::CollisionType::VERTICAL)
						interac->setLastCell(maybeCell);
					else
						interac->setLastCell(getCellAt(interac->getLastCell()->getCol(), maybeCell->getRow()));
				}
				else if (collision == Room::CollisionType::VERTICAL) {
					if (interac->getLastCollision() == Room::CollisionType::HORIZONTAL)
						interac->setLastCell(maybeCell);
					else
						interac->setLastCell(getCellAt(maybeCell->getCol(), interac->getLastCell()->getRow()));
				}
				interac->setLastCollision(collision);
				break;
			}
		}
	}
}

Room::CollisionType InteractiveGrid::resizeRoomUntilCollision(Room* room, GridCell* startCell, GridCell* lastCell, GridCell* currentCell) {
	// Handle DEGENERATED rooms by update each cell (typically low number of cells)
	if (startCell == lastCell || !room->isValid()) {
		room->clear();
		return room->spanFromTo(startCell, currentCell) ? Room::CollisionType::NONE : Room::CollisionType::BOTH;
		//TODO Invalid rooms that collide are not rendered
	}
	// Handle OK rooms by update only changed cells (room cell number can be very high)
	bool isNotCollided_H = true;
	bool isNotCollided_V = true;
	// Compute size delta
	size_t colDist = lastCell->getColDistanceTo(currentCell);
	size_t rowDist = lastCell->getRowDistanceTo(currentCell);
	// Horizontal grow or shrink
	if (startCell->isWestOf(lastCell)) {
		if (lastCell->isWestOf(currentCell))
			isNotCollided_H = room->growToEast(colDist);
		else if (lastCell->isEastOf(currentCell)) {
			if (startCell->isEastOf(currentCell)) {
				room->shrinkToWest(lastCell->getColDistanceTo(startCell));
				isNotCollided_H = room->growToWest(startCell->getColDistanceTo(currentCell));
			} else
				room->shrinkToWest(colDist);
		}
	}
	else if (startCell->isEastOf(lastCell)) {
		if (lastCell->isEastOf(currentCell))
			isNotCollided_H = room->growToWest(colDist);
		else if (lastCell->isWestOf(currentCell)) {
			if (startCell->isWestOf(currentCell)) {
				room->shrinkToEast(lastCell->getColDistanceTo(startCell));
				isNotCollided_H = room->growToEast(startCell->getColDistanceTo(currentCell));
			} else
				room->shrinkToEast(colDist);
		}
	}
	// Vertical grow or shrink
	if (startCell->isNorthOf(lastCell)) {
		if (lastCell->isNorthOf(currentCell))
			isNotCollided_V = room->growToSouth(rowDist);
		else if (lastCell->isSouthOf(currentCell)) {
			if (startCell->isSouthOf(currentCell)) {
				room->shrinkToNorth(lastCell->getRowDistanceTo(startCell));
				isNotCollided_V = room->growToNorth(startCell->getRowDistanceTo(currentCell));
			} else
				room->shrinkToNorth(rowDist);
		}
	}
	else if (startCell->isSouthOf(lastCell)) {
		if (lastCell->isSouthOf(currentCell))
			isNotCollided_V = room->growToNorth(rowDist);
		else if (lastCell->isNorthOf(currentCell)) {
			if (startCell->isNorthOf(currentCell)) {
				room->shrinkToSouth(lastCell->getRowDistanceTo(startCell));
				isNotCollided_V = room->growToSouth(startCell->getRowDistanceTo(currentCell));
			} else
				room->shrinkToSouth(rowDist);
		}
	}
	// Has room collided?
	if (isNotCollided_H && isNotCollided_V) {
		return Room::CollisionType::NONE;
	}
	else if (isNotCollided_H) {
		return Room::CollisionType::VERTICAL;
	}
	else if (isNotCollided_V) {
		return Room::CollisionType::HORIZONTAL;
	}
	else {
		return Room::CollisionType::BOTH;
	}
}

void InteractiveGrid::buildAt(size_t col, size_t row, GridCell::BuildState buildState) {
	GridCell* maybeCell = getCellAt(col, row);
	if (!maybeCell) return;
	if (maybeCell->getBuildState() == buildState) return;
	if (meshpool_) {
		RoomSegmentMesh::InstanceBufferRange bufferRange;
		if (maybeCell->getBuildState() == GridCell::BuildState::EMPTY) {
			// Add instance, if cell was empty
			RoomSegmentMesh::Instance instance;
			instance.scale = glm::vec3(model_matrix_ * glm::vec4(cell_size_)) / 2.0f; //TODO Find the right scale!
			instance.translation = glm::vec3(model_matrix_ * glm::vec4(maybeCell->getPosition() + glm::vec2(cell_size_/2.0f, -cell_size_/2.0f), 0.0f, 1.0f));
			bufferRange = meshpool_->addInstanceUnordered(buildState, instance);
			maybeCell->setMeshInstance(bufferRange);
		}
		else if (buildState == GridCell::BuildState::EMPTY) {
			// Remove instance, if cell is going to be empty
			bufferRange = maybeCell->getMeshInstance();
			bufferRange.mesh_->removeInstanceUnordered(bufferRange.offset_instances_);
		}
		else {
			// Alter instance, if build states change between others than empty
			bufferRange = maybeCell->getMeshInstance();
			bufferRange.mesh_->removeInstanceUnordered(bufferRange.offset_instances_);
			RoomSegmentMesh::Instance instance;
			instance.scale = glm::vec3(model_matrix_ * glm::vec4(cell_size_)) / 2.0f; //TODO Find the right scale!
			instance.translation = glm::vec3(model_matrix_ * glm::vec4(maybeCell->getPosition() + glm::vec2(cell_size_/2.0f, -cell_size_/2.0f), 0.0f, 1.0f));
			bufferRange = meshpool_->addInstanceUnordered(buildState, instance);
			maybeCell->setMeshInstance(bufferRange);
		}
	}
	maybeCell->updateBuildState(vbo_, buildState);
}

void InteractiveGrid::setRoomSegmentMeshPool(RoomSegmentMeshPool* meshpool) {
	meshpool_ = meshpool;
}

RoomSegmentMeshPool* InteractiveGrid::getRoomSegmentMeshPool() {
	return meshpool_;
}

bool InteractiveGrid::isColumnEmptyBetween(size_t col, size_t startRow, size_t endRow) {
	if (endRow < startRow) {
		size_t tmp = endRow;
		endRow = startRow;
		startRow = tmp;
	}
	GridCell* maybeCell = 0;
	for (size_t i = startRow; i <= endRow; i++) {
		maybeCell = getCellAt(col, i);
		if (!maybeCell)
			return false;
		if (maybeCell->getBuildState() != GridCell::BuildState::EMPTY)
			return false;
	}
	return true;
}

bool InteractiveGrid::isRowEmptyBetween(size_t row, size_t startCol, size_t endCol) {
	if (endCol < startCol) {
		size_t tmp = endCol;
		endCol = startCol;
		startCol = tmp;
	}
	GridCell* maybeCell = 0;
	for (size_t j = startCol; j <= endCol; j++) {
		maybeCell = getCellAt(j, row);
		if (!maybeCell)
			return false;
		if (maybeCell->getBuildState() != GridCell::BuildState::EMPTY)
			return false;
	}
	return true;
}

float InteractiveGrid::getCellSize() {
	return cell_size_;
}

size_t InteractiveGrid::getNumColumns() {
	return cells_.size();
}

size_t InteractiveGrid::getNumRows() {
	return cells_[0].size();
}

size_t InteractiveGrid::getNumCells() {
	return cells_.size() * cells_[0].size();
}