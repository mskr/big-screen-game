#include "MeshInstanceGrid.h"

MeshInstanceGrid::MeshInstanceGrid(size_t columns, size_t rows, float height, RoomSegmentMeshPool* meshpool) :
	RoomInteractiveGrid(columns, rows, height), meshpool_(meshpool)
{

}

void MeshInstanceGrid::addInstanceAt(GridCell* c, GridCell::BuildState st) {
	RoomSegmentMesh* mesh = meshpool_->getMeshOfType(st);
	if (!mesh) return;
	RoomSegmentMesh::Instance instance;
	instance.scale = cell_size_ / 2.0f;
	instance.translation = translation_; // grid translation + relative cell translation
	instance.translation += glm::vec3(c->getPosition() + glm::vec2(cell_size_ / 2.0f, -cell_size_ / 2.0f), 0.0f);
	instance.buildState = st;
	instance.health = c->getHealthPoints();
	c->setMeshInstance(mesh->addInstanceUnordered(instance));
}

void MeshInstanceGrid::removeInstanceAt(GridCell* c) {
	RoomSegmentMesh::InstanceBufferRange bufferRange = c->getMeshInstance();
	if (bufferRange.mesh_)
		bufferRange.mesh_->removeInstanceUnordered(bufferRange.offset_instances_);
}

void MeshInstanceGrid::buildAt(GridCell* c, GridCell::BuildState newSt) {
	GridCell::BuildState current = (GridCell::BuildState)c->getBuildState();
	if (current == newSt) return;
	else if (current == GridCell::BuildState::EMPTY) addInstanceAt(c, newSt);
	else if (newSt == GridCell::BuildState::EMPTY) removeInstanceAt(c);
	else {
		removeInstanceAt(c);
		addInstanceAt(c, newSt);
	}
	c->updateBuildState(vbo_, newSt);
}

void MeshInstanceGrid::buildAt(size_t col, size_t row, GridCell::BuildState buildState) {
	GridCell* maybeCell = getCellAt(col, row);
	if (maybeCell) buildAt(maybeCell, buildState);
}