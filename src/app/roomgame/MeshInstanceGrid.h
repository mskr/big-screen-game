#pragma once

namespace roomgame
{
    class RoomSegmentMeshPool;
    class InteractiveGrid;
    class AutomatonGrid;
    /* Represents a grid with mesh instances at its cells
    * Has InteractiveGrid as base class.
    * Adds possibility to add/remove mesh instances through buildAt function.
    * Uses a meshpool to request instanced meshes.
    * Calls addInstance on requested meshes.
    */
    class MeshInstanceGrid {
    protected:
        RoomSegmentMeshPool* meshpool_;
        void addInstanceAt(GridCell*, GLuint);
        void removeInstanceAt(GridCell*);
    public:
        std::shared_ptr<InteractiveGrid> interactiveGrid_;
        AutomatonGrid* automatonGrid_;
        enum BuildMode {
            Additive = 0,
            Replace = 1,
            RemoveSpecific = 2
        };

        bool deleteNeighbouringWalls(GridCell* cell, bool simulate);
        MeshInstanceGrid(RoomSegmentMeshPool* meshpool);
        void buildAt(size_t col, size_t row, GLuint newState, BuildMode buildMode);
        void buildAt(size_t col, size_t row, std::function<void(GridCell*)> callback);
        void buildAt(GridCell*, GLuint newState, BuildMode buildMode);
        void buildAt(GridCell*, std::function<void(GridCell*)> callback);
    };
}