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
        room->updateCorners(touchedCell, touchedCell);
        buildAt(touchedCell->getCol(), touchedCell->getRow(), GridCell::INVALID | GridCell::TEMPORARY, InteractiveGrid::BuildMode::Additive); // room covering one cell is invalid
    }
    // check if infected or source
    else if (touchedCell->getBuildState() & (GridCell::SOURCE | GridCell::INFECTED)) {
        //std::cout << "Touched infected cell" << std::endl;
        GLuint north = touchedCell->getNorthNeighbor()->getBuildState();
        GLuint east = touchedCell->getEastNeighbor()->getBuildState();
        GLuint south = touchedCell->getSouthNeighbor()->getBuildState();
        GLuint west = touchedCell->getWestNeighbor()->getBuildState();

        GLuint test = GridCell::SOURCE | GridCell::INFECTED;

        GLuint currentHealth = touchedCell->getHealthPoints();
        GLuint updatedHealth = min(currentHealth + ((GridCell::MAX_HEALTH - GridCell::MIN_HEALTH) / 4),GridCell::MAX_HEALTH);

        if ((north & test) & (south & test) & (east & test) & (west & test) ) {
            //std::cout << "Cell is in the middle of 4 infected cells" << std::endl;
            return;
        }
        else if (touchedCell->getBuildState() & GridCell::SOURCE) {
            if ((north & test) | (south & test) | (east & test) | (west & test)) {
                //std::cout << "Try to cure Source Cell but there are infected cells nerby" << std::endl;
            }
            else {
                //std::cout << "Cure source Cell" << std::endl;
                touchedCell->updateHealthPoints(vbo_, updatedHealth);
                if (currentHealth >= GridCell::MAX_HEALTH) {
                    buildAt(touchedCell->getCol(), touchedCell->getRow(), GridCell::SOURCE | GridCell::INFECTED, InteractiveGrid::BuildMode::RemoveSpecific);
                }
                
            }
        }
        else {
            //std::cout << "Cure infected Cell" << std::endl;
            touchedCell->updateHealthPoints(vbo_, updatedHealth);
            if (currentHealth >= GridCell::MAX_HEALTH) {
                buildAt(touchedCell->getCol(), touchedCell->getRow(), GridCell::INFECTED, InteractiveGrid::BuildMode::RemoveSpecific);
            }
        }
        
    }
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

void RoomInteractiveGrid::checkForNearInfections(Room* newRoom)
{
    //Check for infected neighbouring rooms
    bool infectedNeighbours = false;
    int left2neighbour = static_cast<int>(newRoom->leftLowerCorner_->getCol() - 1);
    int bottom2neighbour = static_cast<int>(newRoom->leftLowerCorner_->getRow() - 1);
    int right2neighbour = static_cast<int>(newRoom->rightUpperCorner_->getCol() + 1);
    int top2neighbour = static_cast<int>(newRoom->rightUpperCorner_->getRow() + 1);

    forEachCellInRange(&cells_[left2neighbour][bottom2neighbour], &cells_[right2neighbour][top2neighbour], static_cast<std::function<void(GridCell*)>>([&](GridCell* cell)
    {
        int column = static_cast<int>(cell->getCol());
        int row = static_cast<int>(cell->getRow());
        if (column == left2neighbour || column == right2neighbour || row == top2neighbour || row == bottom2neighbour)
        {
            if ((cell->getBuildState() & GridCell::INFECTED) != 0)
            {
                infectedNeighbours = true;
            }
        }
    }));
    newRoom->infectedNeighbours = infectedNeighbours;
}

bool RoomInteractiveGrid::checkRoomPosition(Room* newRoom) {
    checkForNearInfections(newRoom);

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
        forEachCellInRange(interac->getRoom()->leftLowerCorner_, interac->getRoom()->rightUpperCorner_, static_cast<std::function<void(GridCell*)>>([&](GridCell* cell) {
            deleteNeighbouringWalls(cell, false);
        }));
    }

    interactions_.remove(interac);
}