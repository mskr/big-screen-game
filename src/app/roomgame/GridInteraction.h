#ifndef GRID_INTERACTION_H_
#define GRID_INTERACTION_H_

#include <sgct/Engine.h>
#include "GridCell.h"

class GridInteraction {
	int touchID_;
	bool is_touched_;
	glm::dvec2 last_position_;
	double last_timestamp_;
	GridCell* start_cell_;
public:
	GridInteraction(int touchID, glm::dvec2 position, GridCell* start_cell);
	~GridInteraction() = default;
	void onRelease(glm::dvec2 position);
	bool testTemporalAndSpatialProximity(GridInteraction* other);
	GridCell* getStartCell();
	int getTouchID();
};

#endif