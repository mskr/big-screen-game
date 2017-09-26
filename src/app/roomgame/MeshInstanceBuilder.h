#pragma once

namespace roomgame
{
    class RoomSegmentMeshPool;
    class InteractiveGrid;
    class AutomatonGrid;
    /* 
     * Class that manages changing the shown meshes
     * Offers several buildAt functions for this. 
    * Uses a meshpool to request instanced meshes.
    * Calls addInstance on requested meshes.
    */
    class MeshInstanceBuilder {
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
        MeshInstanceBuilder(RoomSegmentMeshPool* meshpool);
        void buildAt(size_t col, size_t row, GLuint newState, BuildMode buildMode);
        void buildAt(size_t col, size_t row, std::function<void(GridCell*)> callback);
        void buildAt(GridCell*, GLuint newState, BuildMode buildMode);
        void buildAt(GridCell*, std::function<void(GridCell*)> callback);
    };
}