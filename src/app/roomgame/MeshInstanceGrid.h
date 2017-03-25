#ifndef MESH_INSTANCE_GRID
#define MESH_INSTANCE_GRID

#include "RoomInteractiveGrid.h"

class RoomSegmentMeshPool;
#include "RoomSegmentMeshPool.h"

class MeshInstanceGrid : public RoomInteractiveGrid {
	RoomSegmentMeshPool* meshpool_;
	void addInstanceAt(GridCell*, GridCell::BuildState);
	void removeInstanceAt(GridCell*);
public:
	MeshInstanceGrid(size_t columns, size_t rows, float height, RoomSegmentMeshPool* meshpool);
	virtual void buildAt(size_t col, size_t row, GridCell::BuildState buildState) override;
};

#endif