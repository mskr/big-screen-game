#include "AutomatonGrid.h"
#include "GPUCellularAutomaton.h"

AutomatonGrid::AutomatonGrid(size_t cols, size_t rows, float height, RoomSegmentMeshPool* meshpool) :
	MeshInstanceGrid(cols, rows, height, meshpool)
{
	automaton_ = 0;
	delayed_update_list_ = 0;
}

AutomatonGrid::~AutomatonGrid() {
	DelayedUpdate* dup = delayed_update_list_;
	while (dup) {
		DelayedUpdate* next = dup->next_;
		delete dup;
		dup = next;
	}
}

void AutomatonGrid::setCellularAutomaton(GPUCellularAutomaton* automaton) {
	automaton_ = automaton;
}

void AutomatonGrid::onMeshpoolInitialized() {
}

void AutomatonGrid::buildAt(size_t col, size_t row, GLuint state) {
    // Called on user input (grid update -> automaton update)

    GridCell* c = getCellAt(col, row);
    if (!c) return;
    MeshInstanceGrid::buildAt(c, state);
    c->updateHealthPoints(vbo_, GridCell::MAX_HEALTH);
    // Route results to automaton
    automaton_->updateCell(c, state, c->getHealthPoints());
}

void AutomatonGrid::replaceRoompieceWith(size_t col, size_t row, GLuint state) {
    // Called on user input (grid update -> automaton update)

    GridCell* c = getCellAt(col, row);
    if (!c) return;
    MeshInstanceGrid::replaceRoompieceWith(c, state);
    //c->updateHealthPoints(vbo_, GridCell::MAX_HEALTH);
    // Route results to automaton
    automaton_->updateCell(c, state, c->getHealthPoints());
}

void AutomatonGrid::updateCell(GridCell* c, GLuint state, int hp) {
	// Called on automaton transitions (automaton update -> grid update)

	if (c->getBuildState() == SIMULATED_STATE && state == GridCell::EMPTY) {
		DelayedUpdate* tmp = delayed_update_list_;
		delayed_update_list_ = new DelayedUpdate(1, c, state);
		delayed_update_list_->next_ = tmp;
		return;
	}
	MeshInstanceGrid::buildAt(c, state);
	c->updateHealthPoints(vbo_, hp); // thinking of dynamic outer influence...
	// a fixed-on-cell health is not very practical
}

void AutomatonGrid::onTransition() {
	// Traverse delayed updates from previous transitions...
	DelayedUpdate* dup = delayed_update_list_;
	DelayedUpdate* last = 0;
	while (dup) {
		dup->wait_count_--;
		// ... and perform updates that are due
		if (dup->wait_count_ == 0) {
			MeshInstanceGrid::buildAt(dup->target_, dup->to_);
			if (last) {
				last->next_ = dup->next_;
				delete dup;
				dup = last->next_;
			}
			else {
				delayed_update_list_ = dup->next_;
				delete dup;
				dup = delayed_update_list_;
			}
		}
		else {
			last = dup;
			dup = dup->next_;
		}
	}
}