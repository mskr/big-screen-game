#include "InteractiveGrid.h"


InteractiveGrid::InteractiveGrid(size_t columns, size_t rows, float height) {
	height_units_ = height;
	cell_size_ = height_units_ / float(rows);
	//grid_center_ = glm::vec3(-1.0f + cell_size_ * columns / 2.0f, -1.0f + height_units_ / 2.0f, 0.0f);
    grid_center_ = glm::vec3(0, 0, -4);
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
	translation_ = glm::vec3(-4,-4,0);
	num_vertices_ = 0;
	last_view_projection_ = glm::mat4(1);
}


InteractiveGrid::~InteractiveGrid() {
	for(GridInteraction* ia : interactions_) delete ia;
}


void InteractiveGrid::updateProjection(glm::mat4& p) {
	last_view_projection_ = p;
}


glm::vec2 InteractiveGrid::getNDC(glm::vec2 cellPosition) {
	// Apply grid translation
	glm::vec4 pos(cellPosition.x + translation_.x, cellPosition.y + translation_.y, 0.0f, 1.0f);
	// Apply camera projection
	pos = last_view_projection_ * pos;
	return glm::vec2(pos.x, pos.y) / pos.w;
}

glm::vec3 InteractiveGrid::getWorldCoordinates(glm::vec2 cellPosition) {
	return glm::vec3(cellPosition, 0.0f) + translation_;
}

GridCell* InteractiveGrid::getClosestWallCell(glm::vec2 pos) {
	//TODO: write
	return getCellAt(glm::vec2(0,0));
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

glm::vec2 InteractiveGrid::pushNDCinsideGrid(glm::vec2 positionNDC) {
    GridCell& leftUpperCell = cells_[0][cells_[0].size() - 1];
    glm::vec2 posLeftUpperNDC = getNDC(leftUpperCell.getPosition());
    GridCell& rightLowerCell = cells_[cells_.size() - 1][0];
    glm::vec2 posRightLowerNDC = getNDC(glm::vec2(
        rightLowerCell.getXPosition() + cell_size_, rightLowerCell.getYPosition() - cell_size_));

    positionNDC.x = glm::min<float>(glm::max<float>(posLeftUpperNDC.x, positionNDC.x), posRightLowerNDC.x);
    positionNDC.y = glm::min<float>(glm::max<float>(posRightLowerNDC.y, positionNDC.y), posLeftUpperNDC.y);
    return positionNDC;
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


GridCell* InteractiveGrid::getCellAt(glm::vec2 positionNDC) {
	if (!isInsideGrid(positionNDC))
		return 0;
	size_t iLeftUpper = 0;
	size_t jLeftUpper = cells_[0].size() - 1;
	size_t iRightLower = cells_.size() - 1;
	size_t jRightLower = 0;
	// dividing grid until searched cell is in a very small subgrid...
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
	// ...then brute force search the subgrid
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

GridCell* InteractiveGrid::pickCell(glm::vec3 rayStartPoint, glm::vec3 rayIntermediatePoint) {
	// intersecting ray with grid plane
	glm::mat3 m{ 0.0f };
	m[0] = rayStartPoint - rayIntermediatePoint;
	glm::vec3 center = grid_center_;
    glm::vec3 gridLeftLowerCorner = getWorldCoordinates(cells_[0][0].getPosition());
    glm::vec3 gridRight = getWorldCoordinates(cells_[getNumColumns()-1][0].getPosition())- gridLeftLowerCorner;
    glm::vec3 gridTop = getWorldCoordinates(cells_[0][getNumRows() - 1].getPosition())- gridLeftLowerCorner;

	m[1] = gridRight;
	m[2] = gridTop;

	glm::vec3 intersection = glm::inverse(m) * (rayStartPoint - gridLeftLowerCorner);
    //std::cout << "rayStartPoint: " << rayStartPoint.x << "/" << rayStartPoint.y << "/" << rayStartPoint.z << std::endl;
    //std::cout << "rayIntermediatePoint: " << rayIntermediatePoint.x << "/" << rayIntermediatePoint.y << "/" << rayIntermediatePoint.z << std::endl;
    //std::cout << "gridTop: " << gridTop.x << "/" << gridTop.y << "/" << gridTop.z << std::endl;
    //std::cout << "gridRight: " << gridRight.x << "/" << gridRight.y << "/" << gridRight.z << std::endl;
    //std::cout << "Intersection: " << intersection.x << "/" << intersection.y << "/" << intersection.z << std::endl;
    //std::cout << "Lower Left: " << gridLeftLowerCorner.x << "/" << gridLeftLowerCorner.y << "/" << gridLeftLowerCorner.z << std::endl;

    size_t row = (size_t)glm::round(intersection.z * getNumRows());
    size_t col = (size_t)glm::round(intersection.y * getNumColumns());

	if (row >= getNumRows() || col >= getNumColumns()) return 0;
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


void InteractiveGrid::loadShader(viscom::GPUProgramManager mgr, std::shared_ptr<viscom::GPUProgram> inst, std::shared_ptr<viscom::GPUProgram> terrain) {
	glEnable(GL_PROGRAM_POINT_SIZE);
	shader_ = mgr.GetResource("viewBuildStates",
		std::initializer_list<std::string>{ "viewBuildStates.vert", "viewBuildStates.frag" });
    instanceShader_ = inst;
    terrainShader_ = terrain;
    //instanceShader_ = mgr.GetResource("renderMeshInstance",
    //    std::initializer_list<std::string>{ "renderMeshInstance.vert", "renderMeshInstance.frag" });
    //terrainShader_ = mgr.GetResource("underwater",
    //    std::initializer_list<std::string>{ "underwater.vert", "underwater.frag" });
    mvp_uniform_location_ = shader_->getUniformLocation("MVP");
}


void InteractiveGrid::onFrame() {
	// Debug render
	glDisable(GL_DEPTH_TEST);
	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glUseProgram(shader_->getProgramId());
	last_view_projection_[3] += glm::vec4(translation_, 0);
	glUniformMatrix4fv(mvp_uniform_location_, 1, GL_FALSE, glm::value_ptr(last_view_projection_));
	glDrawArrays(GL_POINTS, 0, num_vertices_);
	glEnable(GL_DEPTH_TEST);
}


void InteractiveGrid::cleanup() {
	glDeleteBuffers(1, &vbo_);
	glDeleteVertexArrays(1, &vao_);
}

// TODO TEST What happens with concurrent input? Multitouch!?


void InteractiveGrid::onTouch(int touchID) {
	// now use picking
	GridCell* maybeCell = pickCell(last_ray_start_point_, last_ray_intermediate_point_);
	if (!maybeCell) return;
	handleTouchedCell(touchID, maybeCell);
}


void InteractiveGrid::onRelease(int touchID) {
	for (GridInteraction* interac : interactions_) {
		if (interac->getTouchID() == touchID) {
			handleRelease(interac);
			break;
		}
	}
}

void InteractiveGrid::onMouseMove(int touchID, glm::vec3 rayStartPoint, glm::vec3 rayIntermediatePoint) {
	last_ray_start_point_ = rayStartPoint;
	last_ray_intermediate_point_ = rayIntermediatePoint;
	for (GridInteraction* interac : interactions_) {
		if (interac->getTouchID() == touchID) {
			GridCell* maybeCell = pickCell(last_ray_start_point_, last_ray_intermediate_point_);
			if (!maybeCell)	return; // cursor was outside grid
			handleHoveredCell(maybeCell, interac);
			break;
		}
	}
}


void InteractiveGrid::buildAt(size_t col, size_t row, std::function<void(GridCell*)> callback) {
    std::cout << "Called the wrong buildAt" << std::endl;
    //   GridCell* maybeCell = getCellAt(col, row);
    //if (!maybeCell) return;
    //if (maybeCell->getBuildState() == buildState) return;
    //maybeCell->addBuildState(vbo_, buildState);
}

void InteractiveGrid::buildAt(size_t col, size_t row, GLuint state, BuildMode buildMode) {
    std::cout << "Called the wrong buildAt" << std::endl;
    //   GridCell* maybeCell = getCellAt(col, row);
    //if (!maybeCell) return;
    //if (maybeCell->getBuildState() == buildState) return;
    //maybeCell->addBuildState(vbo_, buildState);
}

bool InteractiveGrid::deleteNeighbouringWalls(GridCell* cell, bool simulate) {
    GLuint cellState = cell->getBuildState();
    if ((cellState & GridCell::WALL) != 0) {
        GridCell* neighbour = nullptr;
        GLuint neighbourCellState;
        if ((cellState & GridCell::RIGHT) != 0) {
            neighbour = cell->getEastNeighbor();
            if (neighbour == nullptr) {
                return false;
            }
            neighbourCellState = neighbour->getBuildState();
            if ((neighbourCellState & GridCell::WALL) == 0) {
                neighbour = nullptr;
            }
        }
        else if ((cellState & GridCell::LEFT) != 0) {
            neighbour = cell->getWestNeighbor();
            if (neighbour == nullptr) {
                return false;
            }
            neighbourCellState = neighbour->getBuildState();
            if ((neighbourCellState & GridCell::WALL) == 0) {
                neighbour = nullptr;
            }
        }
        else if ((cellState & GridCell::TOP) != 0) {
            neighbour = cell->getNorthNeighbor();
            if (neighbour == nullptr) {
                return false;
            }
            neighbourCellState = neighbour->getBuildState();
            if ((neighbourCellState & GridCell::WALL) == 0) {
                neighbour = nullptr;
            }
        }
        else if ((cellState & GridCell::BOTTOM) != 0) {
            neighbour = cell->getSouthNeighbor();
            if (neighbour == nullptr) {
                return false;
            }
            neighbourCellState = neighbour->getBuildState();
            if ((neighbourCellState & GridCell::WALL) == 0) {
                neighbour = nullptr;
            }
        }
        if (neighbour != nullptr) {
            if (simulate) {
                return true;
            }
            buildAt(cell->getCol(), cell->getRow(), [&](GridCell* c) {
                GLuint old = c->getBuildState();
                GLuint modified = old & (GridCell::EMPTY | GridCell::INVALID | GridCell::SOURCE | GridCell::INFECTED | GridCell::OUTER_INFLUENCE);
                modified |= GridCell::INSIDE_ROOM;
                c->setBuildState(modified);
            });
            buildAt(neighbour->getCol(), neighbour->getRow(), [&](GridCell* c) {
                GLuint old = c->getBuildState();
                GLuint modified = old & (GridCell::EMPTY | GridCell::INVALID | GridCell::SOURCE | GridCell::INFECTED | GridCell::OUTER_INFLUENCE);
                modified |= GridCell::INSIDE_ROOM;
                c->setBuildState(modified);
            });
            return true;
        }
    }
    return false;
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
		if (maybeCell->getBuildState() != GridCell::EMPTY)
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
		if (maybeCell->getBuildState() != GridCell::EMPTY)
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


void InteractiveGrid::handleTouchedCell(int touchID, GridCell* c) {
	// Debug stuff here, extending classes should add logic
    return;
}

void InteractiveGrid::handleRelease(GridInteraction* in) {
	// Debug stuff here, extending classes should add logic
    return;
}

void InteractiveGrid::handleHoveredCell(GridCell* c, GridInteraction* in) {
	// Debug stuff here, extending classes should add logic
    return;
}