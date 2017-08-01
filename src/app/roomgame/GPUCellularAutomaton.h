#ifndef GPU_CELLULAR_AUTOMATON_H
#define GPU_CELLULAR_AUTOMATON_H

#include "AutomatonGrid.h"
#include "GPUBuffer.h"

/* Implementation of a parallelized cellular automaton.
 * Construct with grid and time between transitions.
 * Applies rules on GPU (see cellularAutomaton.frag).
 * After each transition:
 *  1. Notifies grid by calling onTransition().
 *  2. Copies results to grid.
*/
class GPUCellularAutomaton {
protected:
    AutomatonGrid* grid_;
    std::shared_ptr<viscom::GPUProgram> shader_;
    void copyFromGridToTexture(int tex_index);
    void copyFromTextureToGrid(int tex_index);
private:
    GPUBuffer* framebuffer_pair_[2]; // for double buffering
    GPUBuffer::Tex texture_pair_[2];
    int current_read_index_;
    GLuint* tmp_client_buffer_; // temporary grid state buffer for GPU-CPU transfer
    GLuint vao_; // holds screenfilling quad
    bool is_initialized_; // true if quad and framebuffers are ready
    GLint pixel_size_uniform_location_;
    GLint texture_uniform_location_;
    glm::vec2 pixel_size_;
    double transition_time_;
    double last_time_;
    double delta_time_;
public:
    GPUCellularAutomaton(AutomatonGrid* grid, double transition_time);
    void updateCell(GridCell* c, GLuint state, GLint hp);
    virtual void init(viscom::GPUProgramManager mgr);
    virtual void transition(double time);
    void cleanup();
    //Setter
    void setTransitionTime(double);
    //Getter
    GLfloat getTimeDeltaNormalized();
    GLuint getLatestTexture();
    GLuint getPreviousTexture();
    bool isInitialized();
};

/* Extension of the cellular automaton, that syncs a minimal set of data across SGCT cluster.
 * Synced data:
 * Transition time for interpolating states.
*/
class SynchronizedAutomaton : public GPUCellularAutomaton {
private:
    sgct::SharedFloat shared_time_delta_;
public:
    SynchronizedAutomaton(AutomatonGrid* grid, double transition_time) :
        GPUCellularAutomaton(grid, transition_time)
    {

    }
    virtual void transition(double time) override {
        GPUCellularAutomaton::transition(time);
    }void preSync() { // master
        shared_time_delta_.setVal(GPUCellularAutomaton::getTimeDeltaNormalized());
    }
    void encode() { // master
        sgct::SharedData::instance()->writeFloat(&shared_time_delta_);
    }
    void decode() { // slave
        sgct::SharedData::instance()->readFloat(&shared_time_delta_);
    }
    void updateSyncedSlave() {
        //TODO
    }
    void updateSyncedMaster() {
        //Can maybe stay empty
    }
};

#endif