#include "GridInteraction.h"

GridInteraction::GridInteraction(int touchID, glm::dvec2 position, GridCell* start_cell) {
	touchID_ = touchID;
	is_touched_ = true;
	last_position_ = position;
	last_timestamp_ = glfwGetTime();
	start_cell_ = start_cell;
	last_cell_ = start_cell;
}

void GridInteraction::update(glm::dvec2 position) {
	last_timestamp_ = glfwGetTime();
	last_position_ = position;
}

void GridInteraction::update(GridCell* cell) {
	last_timestamp_ = glfwGetTime();
	last_cell_ = cell;
}

bool GridInteraction::testTemporalAndSpatialProximity(GridInteraction* other) {
	//TODO use this class to resolve discontinuaties
	return false;
}

GridCell* GridInteraction::getStartCell() {
	return start_cell_;
}

GridCell* GridInteraction::getLastCell() {
	return last_cell_;
}

int GridInteraction::getTouchID() {
	return touchID_;
}