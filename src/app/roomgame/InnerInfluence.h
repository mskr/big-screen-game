#ifndef OUTER_INFLUENCE_AUTOMATON_H
#define OUTER_INFLUENCE_AUTOMATON_H

#include "GPUCellularAutomaton.h"

/* Represents the infection spreading throughout rooms
 * Uses a cellular automaton running on the GPU.
 * Infections are initialized by attacking outer influence.
*/
class InnerInfluence : public GPUCellularAutomaton {
    unsigned int num_transitions_;
    const unsigned int NUM_DIRECTIONS_;
    const GLint DEFAULT_FLOW_SPEED = 10;
    const GLint DEFAULT_CRITICAL_VALUE = 20;
public:
	GLint uloc_FLOW_DIRECTION; glm::ivec2* FLOW_DIRECTION;
    GLint uloc_CRITICAL_VALUE; GLint CRITICAL_VALUE;
    GLint uloc_FLOW_SPEED; GLuint FLOW_SPEED;

    InnerInfluence(AutomatonGrid* grid, double transition_time);
    ~InnerInfluence();

	void init(viscom::GPUProgramManager mgr);
	bool transition(double time);
    void Reset();
};

#endif