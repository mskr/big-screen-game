#include "AutomatonGrid.h"
#include "GPUCellularAutomaton.h"

AutomatonGrid::AutomatonGrid(size_t cols, size_t rows, float height, RoomSegmentMeshPool* meshpool) :
	MeshInstanceGrid(cols, rows, height, meshpool)
{
	automaton_ = 0;
}

void AutomatonGrid::setCellularAutomaton(GPUCellularAutomaton* automaton) {
	automaton_ = automaton;
}

void AutomatonGrid::buildAt(size_t col, size_t row, GridCell::BuildState state) {
	MeshInstanceGrid::buildAt(col, row, state);
	GridCell* c = getCellAt(col, row);
	if (!c) return;
	c->updateHealthPoints(vbo_, GridCell::MAX_HEALTH);
	automaton_->updateCell(col, row, state, c->getHealthPoints());
}

void AutomatonGrid::updateGridOnly(size_t col, size_t row, GridCell::BuildState state, int hp) {
	MeshInstanceGrid::buildAt(col, row, state);
	GridCell* c = getCellAt(col, row);
	if (!c) return;
	c->updateHealthPoints(vbo_, hp);
}