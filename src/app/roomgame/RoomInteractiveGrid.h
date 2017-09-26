#ifndef ROOM_INTERACTIVE_GRID
#define ROOM_INTERACTIVE_GRID

#include "InteractiveGrid.h"
#include "Room.h"
#include "SourceLightManager.h"

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
    void ResetHealAmount();
    std::shared_ptr<roomgame::SourceLightManager> sourceLightManager_ = nullptr;
    bool checkRoomPosition(Room* newRoom);
    RoomInteractiveGrid(size_t columns, size_t rows, float height);
    void updateHealthPoints(GridCell* cell, unsigned int hp);
    void reset();
	~RoomInteractiveGrid();
    float healAmount_;
private:
    const float DEFAULT_HEAL_AMOUNT = 0.25f;
    bool firstRoom = true;
    void checkConnection(Room* newRoom, int lengthX, int lengthY, int posX, int posY);
    void checkForNearInfections(Room * newRoom);
};

#endif