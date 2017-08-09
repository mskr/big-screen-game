#include "RoomInteractiveGrid.h"

RoomInteractiveGrid::RoomInteractiveGrid(size_t columns, size_t rows, float height) :
    InteractiveGrid(columns, rows, height)
{
    
}

RoomInteractiveGrid::~RoomInteractiveGrid() {
    for(Room* r : rooms_) delete r;
}

void RoomInteractiveGrid::handleTouchedCell(int touchID, GridCell* touchedCell) {
    // is the touched cell still empty?
    // ...then start room creation
    if (touchedCell->getBuildState() == GridCell::EMPTY) {
        Room* room = new Room(touchedCell, touchedCell, this);
        interactions_.push_back(new GridInteraction(touchID, touchedCell, room));
        buildAt(touchedCell->getCol(), touchedCell->getRow(), GridCell::INVALID); // room covering one cell is invalid
    }
    // check if infected or source
    else if (touchedCell->getBuildState() & (GridCell::SOURCE | GridCell::INFECTED)) {
        std::cout << "Touched infected cell" << std::endl;
        GLuint north = touchedCell->getNorthNeighbor()->getBuildState();
        GLuint east = touchedCell->getEastNeighbor()->getBuildState();
        GLuint south = touchedCell->getSouthNeighbor()->getBuildState();
        GLuint west = touchedCell->getWestNeighbor()->getBuildState();

        GLuint test = GridCell::SOURCE | GridCell::INFECTED;
        if ((north & test) & (south & test) & (east & test) & (west & test) ) {
            
        }
        
    }
}

void RoomInteractiveGrid::handleHoveredCell(GridCell* hoveredCell, GridInteraction* interac) {
    // continue room creation
    if (interac->getTouchID() == -1) { // if ID is -1, "touch" was mouse click
        if (interac->getLastCell() == hoveredCell) return; // return if cursor was still inside last cell
        // resize room is done with following assumptions:
        // - whether rooms grow or shrink can be inferred from start-, last- and current cell
        // - collisions can only occur for growing rooms
        Room::CollisionType collision = resizeRoomUntilCollision(interac->getRoom(),
            interac->getStartCell(), interac->getLastCell(), hoveredCell);
        if (collision == Room::CollisionType::NONE) {
            interac->setLastCell(hoveredCell);
        }
        else if (interac->getLastCollision() == Room::CollisionType::NONE) {
            interac->setLastCell(hoveredCell);
        }
        else if (collision == Room::CollisionType::HORIZONTAL) {
            if (interac->getLastCollision() == Room::CollisionType::VERTICAL)
                interac->setLastCell(hoveredCell);
            else
                interac->setLastCell(getCellAt(interac->getLastCell()->getCol(), hoveredCell->getRow()));
        }
        else if (collision == Room::CollisionType::VERTICAL) {
            if (interac->getLastCollision() == Room::CollisionType::HORIZONTAL)
                interac->setLastCell(hoveredCell);
            else
                interac->setLastCell(getCellAt(hoveredCell->getCol(), interac->getLastCell()->getRow()));
        }
        interac->setLastCollision(collision);
    }
    else { // if ID not 1, then this was really a touch interaction
        //TODO React to touch interaction
        std::cout << "Currently nothing is done for touch interaction." << std::endl;
        std::cout << "We may want to use the same room building stuff here," << std::endl;
        std::cout << "but need to be prepared for discontinuaties (sudden releases) due to bad touch hardware." << std::endl;
    }
}

void RoomInteractiveGrid::handleRelease(GridInteraction* interac) {
    // check result and remove interaction
    if (interac->getTouchID() == -1) { // if ID is -1, "touch" was mouse click
        Room* room = interac->getRoom();
        if (room->isValid()) { // if room valid, i.e. big enough, then store it
            room->finish();
            rooms_.push_back(room);
        }
        else { // if room invalid, discard
            room->clear();
            delete room;
        }
        interactions_.remove(interac);
    }
    else { // if ID not 1, then this was really a touch interaction
        //TODO @Tobias: React to touch interaction, considering bad tuio touch hardware
        /* wait for possible end of discontinuaty for a fixed time.
           if another touch occurs in spatial and temporal proximity, reuse the touchID.
           if timeout, consider this a user-intended release.
        */
    }
    //TODO optimization: instead of iterating the whole grid here, we could iterate over rooms
    forEachCellInRange(getCellAt(0,0),getCellAt(cells_.size()-1, cells_[0].size()-1), [&](GridCell* cell) {
        //GridCell* tmp = cell;
        deleteNeighbouringWalls(cell);
    });
}

Room::CollisionType RoomInteractiveGrid::resizeRoomUntilCollision(Room* room, GridCell* startCell, GridCell* lastCell, GridCell* currentCell) {
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