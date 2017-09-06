#ifndef ROOM_H
#define ROOM_H

#include "GridCell.h"

class InteractiveGrid;
/* A room spanning over a rectangular area of cells on a grid.
 * Rooms are also represented by build states of cells.
 * Can be interactively created/resized.
 * Creation process is controlled by RoomInteractiveGrid.
 * Grow and shrink functions allow to update a minimal amount of cells.
 * Grow functions can furthermore detect collisions (and return false if collided).
 * The span function simply updates all cells in the room and also reports collisions.
 * Once room is finished, finish() should be called.
 * Finish() tries to apply optimizations based on the assumption of fixed size.
 * Clear() sets all cells in the room to empty.
 * Invalidate() sets all cells in the room to invalid.
 * IsValid() check if the room is bigger than the MIN_SIZE (horizontally and vertically).
*/
class Room {
	InteractiveGrid* grid_;
	bool isFinished_;
	std::vector<RoomSegmentMesh::InstanceBufferRange> mesh_instances_;
public:
    GridCell* leftLowerCorner_;
    GridCell* rightUpperCorner_;
    static const size_t MIN_SIZE = 2;
    static const size_t MAX_SIZE = 50;
	Room(GridCell* leftLowerCorner, GridCell* rightUpperCorner, InteractiveGrid* grid);
	~Room();
	void clear();
    void invalidate();
    void validate();
    bool isValid = false;
    bool collision = false;
    bool connected = false;
    bool infectedNeighbours = false;
    bool checkValidity(bool firstRoom);
	void finish();
	size_t getColSize();
	size_t getRowSize();
    void updateCorners(GridCell* startCell, GridCell* endCell);
    void Room::fillRoom(GridCell* startCell, GridCell* endCell, bool temporary);
};

#endif