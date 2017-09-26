#include "AutomatonGrid.h"
#include "GPUCellularAutomaton.h"
#include "InteractiveGrid.h"
namespace roomgame
{
    AutomatonGrid::AutomatonGrid()
    {
        automaton_ = 0;
        delayed_update_list_ = 0;
    }

    AutomatonGrid::~AutomatonGrid() {
        //DelayedUpdate* dup = delayed_update_list_;
        //while (dup) {
        //    DelayedUpdate* next = dup->next_;
        //    delete dup;
        //    dup = next;
        //}
    }

    void AutomatonGrid::setCellularAutomaton(GPUCellularAutomaton* automaton) {
        automaton_ = automaton;
    }


    void AutomatonGrid::updateAutomatonAt(GridCell* c, GLuint state, GLuint hp) {
        // When the outer influences creates a source, set full fluid level, i.e. zero health
        if (c->getBuildState() & GridCell::SOURCE) {
            c->updateHealthPoints(interactiveGrid_->vbo_, GridCell::MIN_HEALTH); //TODO TEST
        }
        automaton_->updateCell(c, c->getBuildState(), c->getHealthPoints());
    }

    void AutomatonGrid::updateGridAt(GridCell* c, GLuint state, GLuint hp) {
        // Called on automaton transitions (automaton update -> grid update)

        // Delay removals of the simulated state mesh to play remove-animation
        if ((c->getBuildState() & SIMULATED_STATE) && !(state & SIMULATED_STATE)) {
            DelayedUpdate* tmp = delayed_update_list_;
            delayed_update_list_ = new DelayedUpdate(1, c, state);
            delayed_update_list_->next_ = tmp;
            return;
        }

        // Do instant update for all other cases
        meshInstanceBuilder_->buildAt(c, state, MeshInstanceBuilder::BuildMode::Replace);
        c->updateHealthPoints(interactiveGrid_->vbo_, hp); // thinking of dynamic inner influence...
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
                meshInstanceBuilder_->buildAt(dup->target_, dup->to_, MeshInstanceBuilder::BuildMode::Additive);
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

    void AutomatonGrid::populateCircleAtLastMousePosition(int radius) {
        /*
        glm::vec2 touchPositionNDC =
        glm::vec2(last_mouse_position_.x, 1.0 - last_mouse_position_.y)
        * glm::vec2(2.0, 2.0) - glm::vec2(1.0, 1.0);
        GridCell* startCell = getCellAt(touchPositionNDC);*/
        //GridCell* startCell = pickCell(last_ray_start_point_, last_ray_intermediate_point_);
        //if (!startCell) return;
        //for (int x = -radius; x < radius; x++) {
        //    for (int y = -radius; y < radius; y++) {
        //        GridCell* c = getCellAt(startCell->getCol() + x, startCell->getRow() + y);
        //        if (!c) continue;
        //        if (c->getDistanceTo(startCell) > radius) continue;
        //        buildAt(c->getCol(), c->getRow(), SIMULATED_STATE, BuildMode::Additive);
        //    }
        //}
    }
}
