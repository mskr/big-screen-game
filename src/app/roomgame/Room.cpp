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
		grid_->buildAt(cell->getCol(), cell->getRow(), GridCell::TEMPORARY | GridCell::INVALID, InteractiveGrid::BuildMode::RemoveSpecific);
	});
}

void Room::invalidate() {
    grid_->forEachCellInRange(leftLowerCorner_, rightUpperCorner_, [&](GridCell* cell) {
        grid_->buildAt(cell->getCol(), cell->getRow(), GridCell::TEMPORARY | GridCell::INVALID, InteractiveGrid::BuildMode::Additive);
    });
}

void Room::validate() {
    grid_->forEachCellInRange(leftLowerCorner_, rightUpperCorner_, [&](GridCell* cell) {
        grid_->buildAt(cell->getCol(), cell->getRow(), GridCell::TEMPORARY | GridCell::INVALID, InteractiveGrid::BuildMode::RemoveSpecific);
    });
}

bool Room::checkValidity(bool firstRoom) {
    size_t hsize = getColSize();
    size_t vsize = getRowSize();

    bool sizeValid = (hsize >= MIN_SIZE) && (vsize >= MIN_SIZE) && (hsize * vsize <= MAX_SIZE);

    bool connectedToARoom = false;

    //grid_->forEachCellInRange(leftLowerCorner_, rightUpperCorner_, [&](GridCell* cell) {
    //    if ((cell->getBuildState() & (GridCell::WALL))!=0) {
    //        bool deletedANeighbour = false;

    //        deletedANeighbour = grid_->deleteNeighbouringWalls(cell, true);
    //        if (deletedANeighbour) {
    //            connectedToARoom = true;
    //        }
    //    }
    //});
    if (firstRoom) {
       connectedToARoom  = true;
    }
    else {
        connectedToARoom = connected;
    }
    isValid = sizeValid && connectedToARoom && !collision;
    std::cout << firstRoom << sizeValid << connectedToARoom << collision << std::endl;
    if (isValid) {
        validate();
    }
    else {
        invalidate();
    }
	return isValid;
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


void Room::updateCorners(GridCell* startCell, GridCell* endCell) {
    size_t minX = min(startCell->getCol(), endCell->getCol());
    size_t minY = min(startCell->getRow(), endCell->getRow());
    size_t maxX = max(startCell->getCol(), endCell->getCol());
    size_t maxY = max(startCell->getRow(), endCell->getRow());

    leftLowerCorner_ = grid_->getCellAt(minX, minY);
    rightUpperCorner_ = grid_->getCellAt(maxX, maxY);
}

void Room::fillRoom(GridCell* startCell, GridCell* endCell, bool temporary) {
    int temporaryCell = temporary ? GridCell::TEMPORARY : GridCell::EMPTY;
    //get corners
    size_t minX = min(startCell->getCol(), endCell->getCol());
    size_t minY = min(startCell->getRow(), endCell->getRow());
    size_t maxX = max(startCell->getCol(), endCell->getCol());
    size_t maxY = max(startCell->getRow(), endCell->getRow());

    GridCell* leftLowerCorner = grid_->getCellAt(minX,minY);
    GridCell* rightUpperCorner = grid_->getCellAt(maxX, maxY);
    GridCell* leftUpperCorner = grid_->getCellAt(minX, maxY);
    GridCell* rightLowerCorner = grid_->getCellAt(maxX, minY);

    if (temporary) {
        grid_->forEachCellInRange(leftLowerCorner_, rightUpperCorner_, [&](GridCell* cell) {
            grid_->buildAt(cell->getCol(), cell->getRow(), GridCell::TEMPORARY, InteractiveGrid::BuildMode::Additive);
        });
    }
    else {
        // Set build states
        grid_->buildAt(minX, minY, GridCell::LEFT | GridCell::BOTTOM | GridCell::CORNER,InteractiveGrid::BuildMode::Replace);
        grid_->buildAt(maxX, maxY, GridCell::RIGHT | GridCell::TOP | GridCell::CORNER, InteractiveGrid::BuildMode::Replace);
        grid_->buildAt(minX, maxY, GridCell::LEFT | GridCell::TOP | GridCell::CORNER, InteractiveGrid::BuildMode::Replace);
        grid_->buildAt(maxX, minY, GridCell::RIGHT | GridCell::BOTTOM | GridCell::CORNER, InteractiveGrid::BuildMode::Replace);

        grid_->forEachCellInRange(leftUpperCorner->getEastNeighbor(), rightUpperCorner->getWestNeighbor(), [&](GridCell* cell) {
            grid_->buildAt(cell->getCol(), cell->getRow(), GridCell::WALL | GridCell::TOP, InteractiveGrid::BuildMode::Replace);
        });
        grid_->forEachCellInRange(leftLowerCorner->getEastNeighbor(), rightLowerCorner->getWestNeighbor(), [&](GridCell* cell) {
            grid_->buildAt(cell->getCol(), cell->getRow(), GridCell::WALL | GridCell::BOTTOM, InteractiveGrid::BuildMode::Replace);
        });
        grid_->forEachCellInRange(leftLowerCorner->getNorthNeighbor(), leftUpperCorner->getSouthNeighbor(), [&](GridCell* cell) {
            grid_->buildAt(cell->getCol(), cell->getRow(), GridCell::WALL | GridCell::LEFT, InteractiveGrid::BuildMode::Replace);
        });
        grid_->forEachCellInRange(rightLowerCorner->getNorthNeighbor(), rightUpperCorner->getSouthNeighbor(), [&](GridCell* cell) {
            grid_->buildAt(cell->getCol(), cell->getRow(), GridCell::WALL | GridCell::RIGHT, InteractiveGrid::BuildMode::Replace);
        });

        GridCell* insideLeftLower = leftLowerCorner->getEastNeighbor()->getNorthNeighbor();
        GridCell* insideRightUpper = rightUpperCorner->getWestNeighbor()->getSouthNeighbor();
        grid_->forEachCellInRange(insideLeftLower, insideRightUpper, [&](GridCell* cell) {
            grid_->buildAt(cell->getCol(), cell->getRow(), GridCell::INSIDE_ROOM, InteractiveGrid::BuildMode::Replace);
        });
    }
}