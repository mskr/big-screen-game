#ifndef ROOM_H
#define ROOM_H

#include "GridCell.h"

class InteractiveGrid;

class Room {
	GridCell* leftLowerCorner_;
	GridCell* rightUpperCorner_;
	InteractiveGrid* grid_;
public:
	static const size_t MIN_SIZE = 2;
	Room(GridCell* leftLowerCorner, GridCell* rightUpperCorner, InteractiveGrid* grid);
	~Room();
	bool spanFromTo(GridCell* startCell, GridCell* endCell);
	bool growToEast(size_t dist);
	void shrinkToWest(size_t dist);
	bool growToWest(size_t dist);
	void shrinkToEast(size_t dist);
	bool growToSouth(size_t dist);
	void shrinkToNorth(size_t dist);
	bool growToNorth(size_t dist);
	void shrinkToSouth(size_t dist);
	bool isValid();
	void invalidate();
	void clear();
	size_t getColSize();
	size_t getRowSize();
	InteractiveGrid* getGrid();
	
	enum CollisionType {
		HORIZONTAL, VERTICAL, BOTH, NONE
	};
};

#endif