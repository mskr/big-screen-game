#pragma once

#include "Room.h"
#include "SourceLightManager.h"
#include "GridInteraction.h"

namespace roomgame
{
    class InteractiveGrid;
    class MeshInstanceGrid;
    /* Grid class that manages rooms that span over grid cells.
    * Has InteractiveGrid as base class.
    * Adds possibility to create rooms by click and drag.
    * Holds a list of rooms.
    */
    class RoomInteractiveGrid {
    public:
        std::shared_ptr<InteractiveGrid> interactiveGrid_;
        std::shared_ptr<MeshInstanceGrid> meshInstanceGrid_;
        std::vector<Room*> rooms_;
        void handleTouchedCell(int touchID, GridCell*);
        void handleHoveredCell(GridCell*, std::shared_ptr<GridInteraction> interac);
        void handleRelease(std::shared_ptr<GridInteraction> interac);
        void ResetHealAmount();
        std::shared_ptr<roomgame::SourceLightManager> sourceLightManager_ = nullptr;
        bool checkRoomPosition(Room* newRoom);
        RoomInteractiveGrid();
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
}
