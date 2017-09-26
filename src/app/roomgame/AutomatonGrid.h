#pragma once
#include <memory>
#include <functional>
#include "GridCell.h"
#include "MeshInstanceBuilder.h"
namespace roomgame
{
    class GPUCellularAutomaton;
    class InteractiveGrid;
    /* Grid class for cellular automatons.
    * Adds possibility to receive changes
    * a) from user input by overriding buildAt() and
    * b) from cellular automaton by offering updateCell() function.
    */
    class AutomatonGrid {
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

        // Update grid only (called from cellular automaton)
        void updateGridAt(GridCell* c, GLuint state, GLuint hp);
        friend GPUCellularAutomaton; // allow private access


    public:
        // Update automaton (called from user input or outer influence through buildAt)
        void updateAutomatonAt(GridCell* c, GLuint state, GLuint hp);

        std::shared_ptr<MeshInstanceBuilder> meshInstanceBuilder_;
        std::shared_ptr<InteractiveGrid> interactiveGrid_;
        static const GLuint SIMULATED_STATE = GridCell::INFECTED;
        AutomatonGrid();
        ~AutomatonGrid();
        void setCellularAutomaton(GPUCellularAutomaton*);

        void onTransition();
        void populateCircleAtLastMousePosition(int radius);
    };
}
