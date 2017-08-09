#include "Room.h"
#include "InteractiveGrid.h"

Room::Room(GridCell* leftLowerCorner, GridCell* rightUpperCorner, InteractiveGrid* grid) {
	leftLowerCorner_ = leftLowerCorner;
	rightUpperCorner_ = rightUpperCorner;
	grid_ = grid;
	isFinished_ = false;
}

Room::~Room() {
	
}

void Room::clear() {
	grid_->forEachCellInRange(leftLowerCorner_, rightUpperCorner_, [&](GridCell* cell) {
		grid_->buildAt(cell->getCol(), cell->getRow(), GridCell::EMPTY);
	});
}

void Room::invalidate() {
	grid_->forEachCellInRange(leftLowerCorner_, rightUpperCorner_, [&](GridCell* cell) {
		grid_->buildAt(cell->getCol(), cell->getRow(), GridCell::INVALID);
	});
}

bool Room::isValid(bool firstRoom) {
	size_t hsize = rightUpperCorner_->getCol() - leftLowerCorner_->getCol();
	size_t vsize = rightUpperCorner_->getRow() - leftLowerCorner_->getRow();

    bool sizeValid = (hsize >= MIN_SIZE) && (vsize >= MIN_SIZE) && (hsize * vsize <= MAX_SIZE);

    bool connectedToARoom = false;

    //int lowerX = max(leftLowerCorner_->getCol() - 1, 0);
    //int lowerY = max(leftLowerCorner_->getRow() - 1, 0);
    //int higherX = min(rightUpperCorner_->getCol() + 1, grid_->getNumColumns() - 1);
    //int higherY = min(rightUpperCorner_->getRow() + 1, grid_->getNumRows() - 1);
    grid_->forEachCellInRange(leftLowerCorner_, rightUpperCorner_, [&](GridCell* cell) {
        bool deletedANeighbour = false;

        deletedANeighbour = grid_->deleteNeighbouringWalls(cell,true);
        if (deletedANeighbour) {
            connectedToARoom = true;
        }
    });
    if (firstRoom) {
        connectedToARoom = true;
    }

	return sizeValid && connectedToARoom;
}

size_t Room::getColSize() {
	return rightUpperCorner_->getCol() - leftLowerCorner_->getCol();
}

size_t Room::getRowSize() {
	return rightUpperCorner_->getRow() - leftLowerCorner_->getRow();
}

void Room::finish() {
	isFinished_ = true;
	//TODO Move mesh instances from unordered to room-ordered buffers (for each mesh that is part of the room)
	// 1) Get instance buffer range for each cell of this room
	// 2) Copy instance to ordered buffer
	// 3) Remove instance from unordered buffer
	// 4) Update cell's mesh instance buffer range
	// 5) Get the whole ordered buffer range and push into mesh_instances_ vector
	// 6) Prevent growing/shrinking/spanning of finished rooms
	// 7) Clearing a finished room should update build state for each cell but remove only 1 buffer range in the ordered buffer! How to do this?
	grid_->forEachCellInRange(leftLowerCorner_, rightUpperCorner_, [&](GridCell* cell) {
        RoomSegmentMesh::InstanceBufferRange bufferRange = cell->getMeshInstance();
	});
}



bool Room::growToEast(size_t dist) {
	GLuint top = GridCell::TOP | GridCell::WALL;
	GLuint bottom = GridCell::BOTTOM | GridCell::WALL;
	GLuint middle = GridCell::INSIDE_ROOM;
	bool isCollisionAhead = false;
	for (size_t i = 0; i <= dist; i++) {
		isCollisionAhead = !grid_->isColumnEmptyBetween(rightUpperCorner_->getCol() + 1,
			leftLowerCorner_->getRow(), rightUpperCorner_->getRow());
		if (i == dist || isCollisionAhead) {
			top = GridCell::TOP | GridCell::RIGHT | GridCell::CORNER;
			bottom = GridCell::BOTTOM | GridCell::RIGHT | GridCell::CORNER;
			middle = GridCell::RIGHT | GridCell::WALL;
		}
		grid_->buildAt(rightUpperCorner_->getCol(), rightUpperCorner_->getRow(), top);
		grid_->buildAt(rightUpperCorner_->getCol(), leftLowerCorner_->getRow(), bottom);
		GridCell* inner = rightUpperCorner_->getSouthNeighbor();
		while (inner->getRow() > leftLowerCorner_->getRow()) {
			grid_->buildAt(inner->getCol(), inner->getRow(), middle);
			inner = inner->getSouthNeighbor();
		}
		if (isCollisionAhead) return false;
		if (i < dist)
			rightUpperCorner_ = rightUpperCorner_->getEastNeighbor();
	}
	return true;
}

void Room::shrinkToWest(size_t dist) {
	GLuint top = GridCell::EMPTY;
	GLuint bottom = GridCell::EMPTY;
	GLuint middle = GridCell::EMPTY;
	if (rightUpperCorner_->getCol() - leftLowerCorner_->getCol() - dist < MIN_SIZE) {
		/*GridCell* empty = rightUpperCorner_;
		while (empty->getRow() >= leftLowerCorner_->getRow()) {
			grid_->buildAt(empty->getCol(), empty->getRow(), middle);
			empty = empty->getSouthNeighbor();
		}*/
		clear();
		rightUpperCorner_ = grid_->getCellAt(rightUpperCorner_->getCol() - dist, rightUpperCorner_->getRow());
		invalidate();
		return;
	}
	for (size_t i = 0; i <= dist; i++) {
		if (i == dist) {
			top = GridCell::RIGHT | GridCell::TOP | GridCell::CORNER;
			bottom = GridCell::RIGHT | GridCell::BOTTOM | GridCell::CORNER;
			middle = GridCell::RIGHT  | GridCell::WALL;
		}
		grid_->buildAt(rightUpperCorner_->getCol(), rightUpperCorner_->getRow(), top);
		grid_->buildAt(rightUpperCorner_->getCol(), leftLowerCorner_->getRow(), bottom);
		GridCell* inner = rightUpperCorner_->getSouthNeighbor();
		while (inner->getRow() > leftLowerCorner_->getRow()) {
			grid_->buildAt(inner->getCol(), inner->getRow(), middle);
			inner = inner->getSouthNeighbor();
		}
		if (i < dist)
			rightUpperCorner_ = rightUpperCorner_->getWestNeighbor();
	}
}

bool Room::growToWest(size_t dist) {
    GLuint top = GridCell::WALL | GridCell::TOP;
    GLuint bottom = GridCell::WALL | GridCell::BOTTOM;
    GLuint middle = GridCell::INSIDE_ROOM;
	bool isCollisionAhead = false;
	for (size_t i = 0; i <= dist; i++) {
		isCollisionAhead = !grid_->isColumnEmptyBetween(leftLowerCorner_->getCol() - 1,
			leftLowerCorner_->getRow(), rightUpperCorner_->getRow());
		if (i == dist || isCollisionAhead) {
			top = GridCell::LEFT | GridCell::TOP | GridCell::CORNER;
			bottom = GridCell::LEFT | GridCell::BOTTOM | GridCell::CORNER;
			middle = GridCell::LEFT | GridCell::WALL;
		}
		grid_->buildAt(leftLowerCorner_->getCol(), rightUpperCorner_->getRow(), top);
		grid_->buildAt(leftLowerCorner_->getCol(), leftLowerCorner_->getRow(), bottom);
		GridCell* inner = leftLowerCorner_->getNorthNeighbor();
		while (inner->getRow() < rightUpperCorner_->getRow()) {
			grid_->buildAt(inner->getCol(), inner->getRow(), middle);
			inner = inner->getNorthNeighbor();
		}
		if (isCollisionAhead) return false;
		if (i < dist)
			leftLowerCorner_ = leftLowerCorner_->getWestNeighbor();
	}
	return true;
}

void Room::shrinkToEast(size_t dist) {
	GLuint top = GridCell::EMPTY;
    GLuint bottom = GridCell::EMPTY;
    GLuint middle = GridCell::EMPTY;
	if (rightUpperCorner_->getCol() - leftLowerCorner_->getCol() - dist < MIN_SIZE) {
		/*GridCell* empty = leftLowerCorner_;
		while (empty->getRow() <= rightUpperCorner_->getRow()) {
			grid_->buildAt(empty->getCol(), empty->getRow(), middle);
			empty = empty->getNorthNeighbor();
		}*/
		clear();
		leftLowerCorner_ = grid_->getCellAt(leftLowerCorner_->getCol() + dist, leftLowerCorner_->getRow());
		invalidate();
		return;
	}
	for (size_t i = 0; i <= dist; i++) {
		if (i == dist) {
            top = GridCell::LEFT | GridCell::TOP | GridCell::CORNER;
            bottom = GridCell::LEFT | GridCell::BOTTOM | GridCell::CORNER;
            middle = GridCell::LEFT | GridCell::WALL;
        }
		grid_->buildAt(leftLowerCorner_->getCol(), rightUpperCorner_->getRow(), top);
		grid_->buildAt(leftLowerCorner_->getCol(), leftLowerCorner_->getRow(), bottom);
		GridCell* inner = leftLowerCorner_->getNorthNeighbor();
		while (inner->getRow() < rightUpperCorner_->getRow()) {
			grid_->buildAt(inner->getCol(), inner->getRow(), middle);
			inner = inner->getNorthNeighbor();
		}
		if (i < dist)
			leftLowerCorner_ = leftLowerCorner_->getEastNeighbor();
	}
}

bool Room::growToSouth(size_t dist) {
	GLuint left = GridCell::WALL | GridCell::LEFT;
	GLuint right = GridCell::WALL | GridCell::RIGHT;
	GLuint middle = GridCell::INSIDE_ROOM;
	bool isCollisionAhead = false;
	for (size_t i = 0; i <= dist; i++) {
		isCollisionAhead = !grid_->isRowEmptyBetween(leftLowerCorner_->getRow() - 1,
			leftLowerCorner_->getCol(), rightUpperCorner_->getCol());
		if (i == dist || isCollisionAhead) {
			left = GridCell::LEFT | GridCell::BOTTOM | GridCell::CORNER;
			right = GridCell::RIGHT | GridCell::BOTTOM | GridCell::CORNER;
			middle = GridCell::WALL | GridCell::BOTTOM;
		}
		grid_->buildAt(leftLowerCorner_->getCol(), leftLowerCorner_->getRow(), left);
		grid_->buildAt(rightUpperCorner_->getCol(), leftLowerCorner_->getRow(), right);
		GridCell* inner = leftLowerCorner_->getEastNeighbor();
		while (inner->getCol() < rightUpperCorner_->getCol()) {
			grid_->buildAt(inner->getCol(), inner->getRow(), middle);
			inner = inner->getEastNeighbor();
		}
		if (isCollisionAhead) return false;
		if (i < dist)
			leftLowerCorner_ = leftLowerCorner_->getSouthNeighbor();
	}
	return true;
}

void Room::shrinkToNorth(size_t dist) {
	GLuint left = GridCell::EMPTY;
	GLuint right = GridCell::EMPTY;
	GLuint middle = GridCell::EMPTY;
	if (rightUpperCorner_->getRow() - leftLowerCorner_->getRow() - dist < MIN_SIZE) {
		/*GridCell* empty = leftLowerCorner_;
		while (empty->getCol() <= rightUpperCorner_->getCol()) {
			grid_->buildAt(empty->getCol(), empty->getRow(), middle);
			empty = empty->getEastNeighbor();
		}*/
		clear();
		leftLowerCorner_ = grid_->getCellAt(leftLowerCorner_->getCol(), leftLowerCorner_->getRow() + dist);
		invalidate();
		return;
	}
	for (size_t i = 0; i <= dist; i++) {
		if (i == dist) {
            left = GridCell::LEFT | GridCell::BOTTOM | GridCell::CORNER;
            right = GridCell::RIGHT | GridCell::BOTTOM | GridCell::CORNER;
            middle = GridCell::WALL | GridCell::BOTTOM;
        }
		grid_->buildAt(leftLowerCorner_->getCol(), leftLowerCorner_->getRow(), left);
		grid_->buildAt(rightUpperCorner_->getCol(), leftLowerCorner_->getRow(), right);
		GridCell* inner = leftLowerCorner_->getEastNeighbor();
		while (inner->getCol() < rightUpperCorner_->getCol()) {
			grid_->buildAt(inner->getCol(), inner->getRow(), middle);
			inner = inner->getEastNeighbor();
		}
		if (i < dist)
			leftLowerCorner_ = leftLowerCorner_->getNorthNeighbor();
	}
}

bool Room::growToNorth(size_t dist) {
	GLuint left = GridCell::WALL | GridCell::LEFT;
	GLuint right = GridCell::WALL | GridCell::RIGHT;
	GLuint middle = GridCell::INSIDE_ROOM;
	bool isCollisionAhead = false;
	for (size_t i = 0; i <= dist; i++) {
		isCollisionAhead = !grid_->isRowEmptyBetween(rightUpperCorner_->getRow() + 1,
			leftLowerCorner_->getCol(), rightUpperCorner_->getCol());
		if (i == dist || isCollisionAhead) {
            left = GridCell::LEFT | GridCell::TOP | GridCell::CORNER;
            right = GridCell::RIGHT | GridCell::TOP | GridCell::CORNER;
            middle = GridCell::WALL | GridCell::TOP;
        }
		grid_->buildAt(leftLowerCorner_->getCol(), rightUpperCorner_->getRow(), left);
		grid_->buildAt(rightUpperCorner_->getCol(), rightUpperCorner_->getRow(), right);
		GridCell* inner = rightUpperCorner_->getWestNeighbor();
		while (inner->getCol() > leftLowerCorner_->getCol()) {
			grid_->buildAt(inner->getCol(), inner->getRow(), middle);
			inner = inner->getWestNeighbor();
		}
		if (isCollisionAhead) return false;
		if (i < dist)
			rightUpperCorner_ = rightUpperCorner_->getNorthNeighbor();
	}
	return true;
}

void Room::shrinkToSouth(size_t dist) {
    GLuint left = GridCell::EMPTY;
    GLuint right = GridCell::EMPTY;
    GLuint middle = GridCell::EMPTY;
    if (rightUpperCorner_->getRow() - leftLowerCorner_->getRow() - dist < MIN_SIZE) {
		/*GridCell* empty = rightUpperCorner_;
		while (empty->getCol() >= leftLowerCorner_->getCol()) {
			grid_->buildAt(empty->getCol(), empty->getRow(), middle);
			empty = empty->getWestNeighbor();
		}*/
		clear();
		rightUpperCorner_ = grid_->getCellAt(rightUpperCorner_->getCol(), rightUpperCorner_->getRow() - dist);
		invalidate();
		return;
	}
	for (size_t i = 0; i <= dist; i++) {
		if (i == dist) {
            left = GridCell::LEFT | GridCell::TOP | GridCell::CORNER;
            right = GridCell::RIGHT | GridCell::TOP | GridCell::CORNER;
            middle = GridCell::WALL | GridCell::TOP;
        }
		grid_->buildAt(leftLowerCorner_->getCol(), rightUpperCorner_->getRow(), left);
		grid_->buildAt(rightUpperCorner_->getCol(), rightUpperCorner_->getRow(), right);
		GridCell* inner = rightUpperCorner_->getWestNeighbor();
		while (inner->getCol() > leftLowerCorner_->getCol()) {
			grid_->buildAt(inner->getCol(), inner->getRow(), middle);
			inner = inner->getWestNeighbor();
		}
		if (i < dist)
			rightUpperCorner_ = rightUpperCorner_->getSouthNeighbor();
	}
}


bool Room::spanFromTo(GridCell* startCell, GridCell* endCell, bool firstRoom) {
	GridCell* leftLowerCorner = 0;
	GridCell* rightUpperCorner = 0;
	GridCell* leftUpperCorner = 0;
	GridCell* rightLowerCorner = 0;
	// Find corners
	if (startCell->getCol() < endCell->getCol()) {
		if (startCell->getRow() < endCell->getRow()) {
			leftLowerCorner = startCell;
			rightUpperCorner = endCell;
			leftUpperCorner = grid_->getCellAt(startCell->getCol(), endCell->getRow());
			rightLowerCorner = grid_->getCellAt(endCell->getCol(), startCell->getRow());
		}
		else {
			leftUpperCorner = startCell;
			rightLowerCorner = endCell;
			leftLowerCorner = grid_->getCellAt(startCell->getCol(), endCell->getRow());
			rightUpperCorner = grid_->getCellAt(endCell->getCol(), startCell->getRow());
		}
	}
	else {
		if (startCell->getRow() < endCell->getRow()) {
			rightLowerCorner = startCell;
			leftUpperCorner = endCell;
			rightUpperCorner = grid_->getCellAt(startCell->getCol(), endCell->getRow());
			leftLowerCorner = grid_->getCellAt(endCell->getCol(), startCell->getRow());
		}
		else {
			rightUpperCorner = startCell;
			leftLowerCorner = endCell;
			rightLowerCorner = grid_->getCellAt(startCell->getCol(), endCell->getRow());
			leftUpperCorner = grid_->getCellAt(endCell->getCol(), startCell->getRow());
		}
	}
	// Test collision by brute force
	bool collision = false;
	grid_->forEachCellInRange(leftLowerCorner, rightUpperCorner, [&](GridCell* cell, bool* found) {
		if (cell->getBuildState() != GridCell::EMPTY) {
			collision = true;
			*found = true;
		}
	});
	if (collision) return false;
	leftLowerCorner_ = leftLowerCorner;
	rightUpperCorner_ = rightUpperCorner;
    size_t hsize = rightUpperCorner_->getCol() - leftLowerCorner_->getCol();
    size_t vsize = rightUpperCorner_->getRow() - leftLowerCorner_->getRow();
	// Test room min size
	if (hsize < Room::MIN_SIZE ||
        vsize < Room::MIN_SIZE ||
        hsize * vsize > Room::MAX_SIZE) {
		grid_->forEachCellInRange(leftLowerCorner, rightUpperCorner, [&](GridCell* cell) {
			grid_->buildAt(cell->getCol(), cell->getRow(), GridCell::INVALID);
		});
		return true;
	}
	// Set build states
	grid_->buildAt(leftLowerCorner->getCol(), leftLowerCorner->getRow(), GridCell::LEFT | GridCell::BOTTOM | GridCell::CORNER);
	grid_->buildAt(rightUpperCorner->getCol(), rightUpperCorner->getRow(), GridCell::RIGHT | GridCell::TOP | GridCell::CORNER);
	grid_->buildAt(leftUpperCorner->getCol(), leftUpperCorner->getRow(), GridCell::LEFT | GridCell::TOP | GridCell::CORNER);
	grid_->buildAt(rightLowerCorner->getCol(), rightLowerCorner->getRow(), GridCell::RIGHT | GridCell::BOTTOM | GridCell::CORNER);
    
	grid_->forEachCellInRange(leftUpperCorner->getEastNeighbor(), rightUpperCorner->getWestNeighbor(), [&](GridCell* cell) {
		grid_->buildAt(cell->getCol(), cell->getRow(), GridCell::WALL | GridCell::TOP);
	});
	grid_->forEachCellInRange(leftLowerCorner->getEastNeighbor(), rightLowerCorner->getWestNeighbor(), [&](GridCell* cell) {
		grid_->buildAt(cell->getCol(), cell->getRow(), GridCell::WALL | GridCell::BOTTOM);
	});
	grid_->forEachCellInRange(leftLowerCorner->getNorthNeighbor(), leftUpperCorner->getSouthNeighbor(), [&](GridCell* cell) {
		grid_->buildAt(cell->getCol(), cell->getRow(), GridCell::WALL | GridCell::LEFT);
	});
	grid_->forEachCellInRange(rightLowerCorner->getNorthNeighbor(), rightUpperCorner->getSouthNeighbor(), [&](GridCell* cell) {
		grid_->buildAt(cell->getCol(), cell->getRow(), GridCell::WALL | GridCell::RIGHT);
	});

	GridCell* insideLeftLower = leftLowerCorner->getEastNeighbor()->getNorthNeighbor();
	GridCell* insideRightUpper = rightUpperCorner->getWestNeighbor()->getSouthNeighbor();
	grid_->forEachCellInRange(insideLeftLower, insideRightUpper, [&](GridCell* cell) {
		grid_->buildAt(cell->getCol(), cell->getRow(), GridCell::INSIDE_ROOM);
	});
	return true;
}
