#ifndef AUTOMATON_GRID
#define AUTOMATON_GRID

#include "MeshInstanceGrid.h"
class GPUCellularAutomaton;

/* Grid class for cellular automatons.
 * Has InteractiveGrid as base class.
 * Adds possibility to receive changes from user input and cellular automaton.
*/
class AutomatonGrid : public MeshInstanceGrid {
	GPUCellularAutomaton* automaton_;
	// Delaying mesh instance updates allows to play smoother animations
	struct DelayedUpdate {
		unsigned int wait_count_; // number of transitions to wait
		GridCell* target_; // cell to update
		GridCell::BuildState to_; // build state to set
		DelayedUpdate* next_; // next list element
		DelayedUpdate(unsigned int wait_count, GridCell* target, GridCell::BuildState to) :
			wait_count_(wait_count), target_(target), to_(to), next_(0) {}
	};
	DelayedUpdate* delayed_update_list_;
public:
	AutomatonGrid(size_t columns, size_t rows, float height, RoomSegmentMeshPool* meshpool);
	~AutomatonGrid();
	void setCellularAutomaton(GPUCellularAutomaton*);
	void onMeshpoolInitialized() override;
	void buildAt(size_t col, size_t row, GridCell::BuildState buildState) override; // for user changes
	void updateCell(GridCell* c, GridCell::BuildState state, int hp); // for automaton changes
	void onTransition();
	void populateCircleAtLastMousePosition(int radius);
};

#endif