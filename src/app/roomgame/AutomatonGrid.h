#ifndef AUTOMATON_GRID
#define AUTOMATON_GRID

#include "MeshInstanceGrid.h"
class GPUCellularAutomaton;

/* Grid class for cellular automatons.
 * Has InteractiveGrid as base class.
 * Inherits functionality of RoomInteractiveGrid and MeshInstanceGrid as well.
 * Adds possibility to receive changes
 * a) from user input by overriding buildAt() and
 * b) from cellular automaton by offering updateCell() function.
*/
class AutomatonGrid : public MeshInstanceGrid {
	GPUCellularAutomaton* automaton_;
	// Delaying mesh instance updates allows to play smoother animations
	struct DelayedUpdate {
		unsigned int wait_count_; // number of transitions to wait
		GridCell* target_; // cell to update
		GLuint to_; // build state to set
		DelayedUpdate* next_; // next list element
		DelayedUpdate(unsigned int wait_count, GridCell* target, GLuint to) :
			wait_count_(wait_count), target_(target), to_(to), next_(0) {}
	};
	DelayedUpdate* delayed_update_list_;
public:
    static const GLuint SIMULATED_STATE = GridCell::INFECTED;
	AutomatonGrid(size_t columns, size_t rows, float height, RoomSegmentMeshPool* meshpool);
	~AutomatonGrid();
	void setCellularAutomaton(GPUCellularAutomaton*);
    void buildAt(size_t col, size_t row, GLuint newState, BuildMode buildMode) override; // for user changes
    void updateCell(GridCell* c, GLuint state, int hp); // for automaton changes
	void onTransition();
	void populateCircleAtLastMousePosition(int radius);
};

#endif