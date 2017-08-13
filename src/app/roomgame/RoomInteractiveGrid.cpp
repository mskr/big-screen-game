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

void RoomInteractiveGrid::checkConnection(Room* newRoom, int lengthX, int lengthY, int posX, int posY) {
    bool connected = false;
    for (Room* r : rooms_) {
        if (r == newRoom) {
            continue;
        }
        if ((posX == (int)(r->leftLowerCorner_->getCol()) - 1) ||
            (posX == (int)(r->rightUpperCorner_->getCol()) + lengthX + 1)) {
            if (posY - (int)r->rightUpperCorner_->getRow() <= lengthY - 2 &&
                posY - (int)r->leftLowerCorner_->getRow() >= 2) {
                connected = true;
                break;
            }
        }
        if ((posY == (int)(r->leftLowerCorner_->getRow()) - 1) ||
            (posY == (int)(r->rightUpperCorner_->getRow()) + lengthY + 1)) {
            if (posX - (int)r->rightUpperCorner_->getCol() <= lengthX - 2 &&
                posX - (int)r->leftLowerCorner_->getCol() >= 2) {
                connected = true;
                break;
            }
        }
    }
    //set result
    newRoom->connected = connected;
}

bool RoomInteractiveGrid::checkRoomPosition(Room* newRoom) {
    int lengthX = (int)newRoom->getColSize();
    int lengthY = (int)newRoom->getRowSize();
    int posX = (int)newRoom->rightUpperCorner_->getCol();
    int posY = (int)newRoom->rightUpperCorner_->getRow();
    //Check for collisions
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
    //If there are no collisions...
    //...check for Connections
    checkConnection(newRoom, lengthX, lengthY, posX, posY);

    newRoom->collision = false;
    return false;
}

void RoomInteractiveGrid::handleHoveredCell(GridCell* hoveredCell, GridInteraction* interac) {
    if (interac->getLastCell() == hoveredCell) return; // return if cursor was still inside last cell
    interac->setLastCell(hoveredCell);
    interac->getRoom()->clear();
    interac->getRoom()->updateCorners(interac->getStartCell(), hoveredCell);
    checkRoomPosition(interac->getRoom());
    interac->getRoom()->checkValidity(firstRoom);
    interac->getRoom()->fillRoom(interac->getStartCell(), interac->getLastCell(), true);

}

void RoomInteractiveGrid::handleRelease(GridInteraction* interac) {
    // check result and remove interaction
    Room* room = interac->getRoom();
    if (room->isValid) { // if room valid, i.e. big enough, non-colliding, connecting, then store it
        if (rooms_.size() == 0) {//First room doesn't need to be connecting
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
        room = nullptr;
    }
    if (room != nullptr) {
        forEachCellInRange(interac->getRoom()->leftLowerCorner_, interac->getRoom()->rightUpperCorner_, [&](GridCell* cell) {
            deleteNeighbouringWalls(cell, false);
        });
    }

    interactions_.remove(interac);
}