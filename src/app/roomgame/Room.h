#ifndef ROOM_H
#define ROOM_H

#include "GridCell.h"

class InteractiveGrid;

class Room {
	GridCell* leftLowerCorner_;
	GridCell* rightUpperCorner_;
	InteractiveGrid* grid_;
	bool isFinished_;
	std::vector<RoomSegmentMesh::InstanceBufferRange> mesh_instances_;
public:
	static const size_t MIN_SIZE = 2;
	Room(GridCell* leftLowerCorner, GridCell* rightUpperCorner, InteractiveGrid* grid);
	~Room();
	void clear();
	void invalidate();
	bool isValid();
	void finish();
	size_t getColSize();
	size_t getRowSize();
	InteractiveGrid* getGrid();

	bool growToEast(size_t dist);
	bool growToWest(size_t dist);
	bool growToSouth(size_t dist);
	bool growToNorth(size_t dist);
	void shrinkToEast(size_t dist);
	void shrinkToWest(size_t dist);
	void shrinkToSouth(size_t dist);
	void shrinkToNorth(size_t dist);
	bool spanFromTo(GridCell* startCell, GridCell* endCell);
	
	enum CollisionType {
		HORIZONTAL, VERTICAL, BOTH, NONE
	};
};

#endif