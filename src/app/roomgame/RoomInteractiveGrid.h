#ifndef ROOM_INTERACTIVE_GRID
#define ROOM_INTERACTIVE_GRID

#include "InteractiveGrid.h"
#include "Room.h"

/* Grid class that manages rooms that span over grid cells.
 * Has InteractiveGrid as base class.
 * Adds possibility to create rooms by click and drag.
 * Holds a list of rooms.
*/
class RoomInteractiveGrid : public InteractiveGrid {
	std::vector<Room*> rooms_;
	void handleTouchedCell(int touchID, GridCell*) override;
	void handleHoveredCell(GridCell*, GridInteraction*) override;
	void handleRelease(GridInteraction*) override;
public:
	RoomInteractiveGrid(size_t columns, size_t rows, float height);
	~RoomInteractiveGrid();
	//Room::CollisionType resizeRoomUntilCollision(Room* room, GridCell* startCell, GridCell* lastCell, GridCell* currentCell);
private:
    bool firstRoom = true;
    GridCell* getNextFreeCell(GridCell* startCell, GridCell* currentCell);
    bool anyRoomCollisions(Room* newRoom);
};

#endif