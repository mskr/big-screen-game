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


void AutomatonGrid::updateGridAt(size_t col, size_t row, GridCell::BuildState state) {

}

void AutomatonGrid::updateAutomatonAt(size_t col, size_t row, GridCell::BuildState state) {
	
}