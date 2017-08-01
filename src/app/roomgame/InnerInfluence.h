#ifndef OUTER_INFLUENCE_AUTOMATON_H
#define OUTER_INFLUENCE_AUTOMATON_H

#include "GPUCellularAutomaton.h"

/* Represents the infection spreading throughout rooms
 * Uses a cellular automaton running on the GPU.
 * Infections are initialized by attacking outer influence.
*/
class InnerInfluence : public SynchronizedAutomaton {
	GLint movedir_uniform_location_;
	glm::ivec2 movedir_;
	GLint birth_thd_uloc_;
	GLfloat birth_thd_;
	GLint death_thd_uloc_;
	GLfloat death_thd_;
	GLint room_nbors_ahead_thd_uloc_; // collision threshold
	GLfloat room_nbors_ahead_thd_;
	GLint outer_infl_nbors_thd_uloc_;
	GLint outer_infl_nbors_thd_;
	GLint damage_per_cell_uloc_;
	GLint damage_per_cell_;
public:
    InnerInfluence(AutomatonGrid* grid, double transition_time);
	void init(viscom::GPUProgramManager mgr);
	void setMoveDir(int x, int y);
	void setBirthThreshold(GLfloat v);
	void setDeathThreshold(GLfloat v);
	void setCollisionThreshold(GLfloat v);
	void setOuterInfluenceNeighborThreshold(GLint v);
	void setDamagePerCell(GLint v);
	void transition(double time);
    void spawnAt(GridCell* cell);
};

#endif