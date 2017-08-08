#ifndef OUTER_INFLUENCE_AUTOMATON_H
#define OUTER_INFLUENCE_AUTOMATON_H

#include "GPUCellularAutomaton.h"

/* Represents the infection spreading throughout rooms
 * Uses a cellular automaton running on the GPU.
 * Infections are initialized by attacking outer influence.
*/
class InnerInfluence : public GPUCellularAutomaton {
public:
	GLint uloc_FLOW_DIRECTION; glm::ivec2 FLOW_DIRECTION;
    GLint uloc_CRITICAL_VALUE; GLint CRITICAL_VALUE;
    GLint uloc_FLOW_SPEED; GLuint FLOW_SPEED;

    InnerInfluence(AutomatonGrid* grid, double transition_time);

	void init(viscom::GPUProgramManager mgr);
	bool transition(double time);
    void spawnAt(GridCell* cell);
};

#endif