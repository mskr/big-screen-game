#include "AutomatonGrid.h"
#include "GPUCellularAutomaton.h"

AutomatonGrid::AutomatonGrid(size_t cols, size_t rows, float height, RoomSegmentMeshPool* meshpool) :
	MeshInstanceGrid(cols, rows, height, meshpool)
{
	automaton_ = 0;
}

void AutomatonGrid::setCellularAutomaton(GPUCellularAutomaton* automaton) {
	automaton_ = automaton;
}

void AutomatonGrid::buildAt(size_t col, size_t row, GridCell::BuildState state) {
	MeshInstanceGrid::buildAt(col, row, state);
	GridCell* c = getCellAt(col, row);
	if (!c) return;
	//c->updateHealthPoints(vbo_, GridCell::MAX_HEALTH); //TODO Health not updated correctly! Need to track cells...
	automaton_->updateCell(col, row, state, c->getHealthPoints());
}

void AutomatonGrid::updateGridOnly(size_t col, size_t row, GridCell::BuildState state, int hp, glm::ivec2 neighborStates) {
	GridCell* c = getCellAt(col, row);
	if (!c) return;
	GridCell::BuildState oldState = (GridCell::BuildState)c->getBuildState();
	MeshInstanceGrid::buildAt(col, row, state);
	c->updateHealthPoints(vbo_, hp);
	if (state != GridCell::BuildState::OUTER_INFLUENCE) return;
	// Provide outer influence mesh instances with information necessary for interpolation:
	// -> Before- and after-transition build states of current cell and its 8 neighbors
	//TODO For room segment mesh instances this information is (probably) not usable
	// Make extra instance struct for outer influence meshes
	RoomSegmentMesh::InstanceBufferRange r = c->getMeshInstance();
	if(oldState == state) // need the last build state even if no change
		RoomSegmentMesh::Instance::updateBuildState(r.buffer_->id_, r.offset_instances_, state);
	// this updates the before-transition neighbors
	RoomSegmentMesh::Instance::updateNeighborBuildStatesPacked(r.buffer_->id_, r.offset_instances_, neighborStates);
}

void AutomatonGrid::setUniformAutomatonTimeDelta(GLfloat t) {
	meshpool_->setUniformAutomatonTimeDelta(t);
}