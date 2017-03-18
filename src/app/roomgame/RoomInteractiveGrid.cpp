#include "RoomInteractiveGrid.h"

RoomInteractiveGrid::RoomInteractiveGrid(size_t columns, size_t rows, float height) :
	InteractiveGrid(columns, rows, height)
{
	
}

RoomInteractiveGrid::~RoomInteractiveGrid() {
	for(Room* r : rooms_) delete r;
}

void RoomInteractiveGrid::onTouch(int touchID) {
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
	buildAt(maybeCell->getCol(), maybeCell->getRow(), GridCell::BuildState::INVALID);
}

void RoomInteractiveGrid::onRelease(int touchID) {
	// Find and remove interaction
	for (GridInteraction* interac : interactions_) {
		if (interac->getTouchID() == touchID) {
			interac->update(last_mouse_position_);
			if (touchID == -1) {
				// React to mouse interaction (id -1)
				Room* room = interac->getRoom();
				if (room->isValid()) {
					// Finish room
					room->finish();
					rooms_.push_back(room);
				}
				else {
					room->clear();
					delete room;
				}
				interactions_.remove(interac);
				break;
			}
			else {
				//TODO React to touch interaction
				//TODO wait for possible end of discontinuaty
			}
		}
	}
}

void RoomInteractiveGrid::onMouseMove(int touchID, double newx, double newy) {
	InteractiveGrid::onMouseMove(touchID, newx, newy);
	// Find interaction and continue room building
	for (GridInteraction* interac : interactions_) {
		if (interac->getTouchID() == touchID) {
			interac->update(last_mouse_position_);
			if (touchID == -1) {
				// React to mouse interaction (id -1)
				glm::vec2 touchPositionNDC =
					glm::vec2(last_mouse_position_.x, 1.0 - last_mouse_position_.y)
					* glm::vec2(2.0, 2.0) - glm::vec2(1.0, 1.0); // transform window system coords
				GridCell* maybeCell = getCellAt(touchPositionNDC); // search current cell
				if (!maybeCell) {
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
			//TODO React to touch interaction
		}
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