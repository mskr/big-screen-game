#include "RoomInteractiveGrid.h"

RoomInteractiveGrid::RoomInteractiveGrid(size_t columns, size_t rows, float height) :
	InteractiveGrid(columns, rows, height)
{
	
}

RoomInteractiveGrid::~RoomInteractiveGrid() {
	for(Room* r : rooms_) delete r;
}

void RoomInteractiveGrid::handleTouchedCell(int touchID, GridCell* touchedCell) {
	InteractiveGrid::handleTouchedCell(touchID, touchedCell);
	// is the touched cell still empty?
	if (touchedCell->getBuildState() != GridCell::EMPTY) return;
	// ...then start room creation
	Room* room = new Room(touchedCell, touchedCell, this);
	interactions_.push_back(new GridInteraction(touchID, last_mouse_position_, touchedCell, room));
	buildAt(touchedCell->getCol(), touchedCell->getRow(), GridCell::INVALID);
}

void RoomInteractiveGrid::handleHoveredCell(GridCell* hoveredCell, GridInteraction* interac) {
	InteractiveGrid::handleHoveredCell(hoveredCell, interac);
	// continue room creation
	if (interac->getTouchID() == -1) { // if ID is -1, "touch" was mouse click
		if (interac->getLastCell() == hoveredCell) return; // return if cursor was still inside last cell
		// resize room 
		// - if rooms grow or shrink can be inferred from start-, last- and current cell
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
	}
}

void RoomInteractiveGrid::handleRelease(GridInteraction* interac) {
	InteractiveGrid::handleRelease(interac);
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
		//TODO React to touch interaction
		//TODO wait for possible end of discontinuaty
	}
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