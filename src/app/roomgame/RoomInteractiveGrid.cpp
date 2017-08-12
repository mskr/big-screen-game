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
    if (touchedCell->getBuildState() != GridCell::EMPTY) return;
    // ...then start room creation
    Room* room = new Room(touchedCell, touchedCell, this);
    interactions_.push_back(new GridInteraction(touchID, touchedCell, room));
    room->updateCorners(touchedCell, touchedCell);
    buildAt(touchedCell->getCol(), touchedCell->getRow(), GridCell::INVALID | GridCell::TEMPORARY, InteractiveGrid::BuildMode::Additive); // room covering one cell is invalid
}

GridCell* RoomInteractiveGrid::getNextFreeCell(GridCell* startCell, GridCell* currentCell) {
    if (currentCell->getBuildState() == GridCell::EMPTY) {
        return currentCell;
    }
    else {
        int diffX = (int)(startCell->getCol() - currentCell->getCol());
        diffX = diffX > 0 ? 1 : (diffX == 0 ? 0 : -1);
        int diffY = (int)(startCell->getRow() - currentCell->getRow());
        diffY = diffY > 0 ? 1 : (diffY == 0 ? 0 : -1);
        if (diffX == 0 && diffY == 0) {
            return currentCell;
        }
        else {
            return getNextFreeCell(startCell, getCellAt(currentCell->getCol() + diffX, currentCell->getRow() + diffY));
        }
    }
}

bool RoomInteractiveGrid::markRoomCollisions(Room* newRoom) {
    int lengthX = newRoom->getColSize();
    int lengthY = newRoom->getRowSize();
    int posX = newRoom->rightUpperCorner_->getCol();
    int posY = newRoom->rightUpperCorner_->getRow();
    for (Room* r : rooms_) {
        if (r == newRoom) {
            continue;
        }
        if ((posX >= (int)(r->leftLowerCorner_->getCol())) && 
            (posX <= (int)(r->rightUpperCorner_->getCol()) + lengthX)) {
            
            if (posY >= r->leftLowerCorner_->getRow() && 
                posY <= r->rightUpperCorner_->getRow() + lengthY) {
                newRoom->collision = true;
                return true;
            }
        }
    }
    bool connected = false;
    for (Room* r : rooms_) {
        if (r == newRoom) {
            continue;
        }
        if ((posX == (int)(r->leftLowerCorner_->getCol()) - 1) ||
            (posX == (int)(r->rightUpperCorner_->getCol()) + lengthX + 1)) {
            if (posY - (int)r->rightUpperCorner_->getRow() < lengthY - 3 &&
                posY - (int)r->leftLowerCorner_->getRow() > 3) {
                connected = true;
                break;
            }
        }
        if ((posY == (int)(r->leftLowerCorner_->getRow()) - 1) ||
            (posY == (int)(r->rightUpperCorner_->getRow()) + lengthY + 1)) {
            if (posX - (int)r->rightUpperCorner_->getCol() < lengthX - 3 &&
                posX - (int)r->leftLowerCorner_->getCol() > 3) {
                connected = true;
                break;
            }
        }
    }
    newRoom->connected = connected;
    newRoom->collision = false;
    return false;
}

void RoomInteractiveGrid::handleHoveredCell(GridCell* hoveredCell, GridInteraction* interac) {
    if (interac->getLastCell() == hoveredCell) return; // return if cursor was still inside last cell
    interac->setLastCell(hoveredCell);
    interac->getRoom()->clear();
    interac->getRoom()->updateCorners(interac->getStartCell(), hoveredCell);
    markRoomCollisions(interac->getRoom());
    interac->getRoom()->checkValidity(firstRoom);
    interac->getRoom()->fillRoom(interac->getStartCell(), interac->getLastCell(), true);

}

void RoomInteractiveGrid::handleRelease(GridInteraction* interac) {
    // check result and remove interaction
    //if (interac->getTouchID() == -1) { // if ID is -1, "touch" was mouse click
    if (true) {
        Room* room = interac->getRoom();
        if (room->isValid) { // if room valid, i.e. big enough, then store it
            if (rooms_.size() == 0) {
                firstRoom = false;
            }
            room->clear();
            interac->getRoom()->fillRoom(interac->getRoom()->leftLowerCorner_, interac->getRoom()->rightUpperCorner_, false);
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
        deleteNeighbouringWalls(cell,false);
    });
}

//Room::CollisionType RoomInteractiveGrid::resizeRoomUntilCollision(Room* room, GridCell* startCell, GridCell* lastCell, GridCell* currentCell) {
//    // Handle DEGENERATED rooms by update each cell (typically low number of cells)
//    if (startCell == lastCell || !room->isValid(firstRoom)) {
//        room->clear();
//        return room->spanFromTo(startCell, currentCell, firstRoom) ? Room::CollisionType::NONE : Room::CollisionType::BOTH;
//        //TODO Invalid rooms that collide are not rendered
//    }
//    // Handle OK rooms by update only changed cells (room cell number can be very high)
//    bool isNotCollided_H = true;
//    bool isNotCollided_V = true;
//    // Compute size delta
//    size_t colDist = lastCell->getColDistanceTo(currentCell);
//    size_t rowDist = lastCell->getRowDistanceTo(currentCell);
//    // Horizontal grow or shrink
//    if (startCell->isWestOf(lastCell)) {
//        if (lastCell->isWestOf(currentCell))
//            isNotCollided_H = room->growToEast(colDist);
//        else if (lastCell->isEastOf(currentCell)) {
//            if (startCell->isEastOf(currentCell)) {
//                room->shrinkToWest(lastCell->getColDistanceTo(startCell));
//                isNotCollided_H = room->growToWest(startCell->getColDistanceTo(currentCell));
//            } else
//                room->shrinkToWest(colDist);
//        }
//    }
//    else if (startCell->isEastOf(lastCell)) {
//        if (lastCell->isEastOf(currentCell))
//            isNotCollided_H = room->growToWest(colDist);
//        else if (lastCell->isWestOf(currentCell)) {
//            if (startCell->isWestOf(currentCell)) {
//                room->shrinkToEast(lastCell->getColDistanceTo(startCell));
//                isNotCollided_H = room->growToEast(startCell->getColDistanceTo(currentCell));
//            } else
//                room->shrinkToEast(colDist);
//        }
//    }
//    // Vertical grow or shrink
//    if (startCell->isNorthOf(lastCell)) {
//        if (lastCell->isNorthOf(currentCell))
//            isNotCollided_V = room->growToSouth(rowDist);
//        else if (lastCell->isSouthOf(currentCell)) {
//            if (startCell->isSouthOf(currentCell)) {
//                room->shrinkToNorth(lastCell->getRowDistanceTo(startCell));
//                isNotCollided_V = room->growToNorth(startCell->getRowDistanceTo(currentCell));
//            } else
//                room->shrinkToNorth(rowDist);
//        }
//    }
//    else if (startCell->isSouthOf(lastCell)) {
//        if (lastCell->isSouthOf(currentCell))
//            isNotCollided_V = room->growToNorth(rowDist);
//        else if (lastCell->isNorthOf(currentCell)) {
//            if (startCell->isNorthOf(currentCell)) {
//                room->shrinkToSouth(lastCell->getRowDistanceTo(startCell));
//                isNotCollided_V = room->growToSouth(startCell->getRowDistanceTo(currentCell));
//            } else
//                room->shrinkToSouth(rowDist);
//        }
//    }
//    // Has room collided?
//    if (isNotCollided_H && isNotCollided_V) {
//        return Room::CollisionType::NONE;
//    }
//    else if (isNotCollided_H) {
//        return Room::CollisionType::VERTICAL;
//    }
//    else if (isNotCollided_V) {
//        return Room::CollisionType::HORIZONTAL;
//    }
//    else {
//        return Room::CollisionType::BOTH;
//    }
//}