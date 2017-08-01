#ifndef GPU_CELLULAR_AUTOMATON_H
#define GPU_CELLULAR_AUTOMATON_H

#include "AutomatonGrid.h"
#include "GPUBuffer.h"

/* Implementation of a parallelized cellular automaton.
 * Construct with grid and time between transitions.
 * Call init() once to setup GPU state.
 * Call transition(time) as often as pleased with current time.
 * transition(time) returns immediately if it is not time yet.
 * Else it applies rules in shader (see cellularAutomaton.frag).
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

#endif