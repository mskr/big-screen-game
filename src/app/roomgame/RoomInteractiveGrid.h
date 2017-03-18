#ifndef ROOM_INTERACTIVE_GRID
#define ROOM_INTERACTIVE_GRID

#include "InteractiveGrid.h"
#include "Room.h"

class RoomInteractiveGrid : public InteractiveGrid {
	std::vector<Room*> rooms_;
public:
	RoomInteractiveGrid(size_t columns, size_t rows, float height);
	~RoomInteractiveGrid();
	void onTouch(int touchID) override;
	void onRelease(int touchID) override;
	void onMouseMove(int touchID, double newx, double newy) override;
	Room::CollisionType resizeRoomUntilCollision(Room* room, GridCell* startCell, GridCell* lastCell, GridCell* currentCell);
};

#endif