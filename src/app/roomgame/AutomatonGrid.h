#ifndef AUTOMATON_GRID
#define AUTOMATON_GRID

#include "MeshInstanceGrid.h"
class GPUCellularAutomaton;

class AutomatonGrid : public MeshInstanceGrid {
	GPUCellularAutomaton* automaton_;
public:
	AutomatonGrid(size_t columns, size_t rows, float height, RoomSegmentMeshPool* meshpool);
	void setCellularAutomaton(GPUCellularAutomaton*);
	void buildAt(size_t col, size_t row, GridCell::BuildState buildState) override;
	void updateGridOnly(size_t col, size_t row, GridCell::BuildState state, int hp);
};

#endif