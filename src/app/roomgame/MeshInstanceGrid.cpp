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

void MeshInstanceGrid::buildAt(size_t col, size_t row, GridCell::BuildState buildState) {
	GridCell* maybeCell = getCellAt(col, row);
	if (!maybeCell) return;
	GridCell::BuildState oldBuildState = (GridCell::BuildState)maybeCell->getBuildState();
	if (oldBuildState == buildState) return;
	if (oldBuildState == GridCell::BuildState::EMPTY) {
		// Add instance, if cell was empty
		addInstanceAt(maybeCell, buildState);
	}
	else if (buildState == GridCell::BuildState::EMPTY) {
		// Remove instance, if cell is going to be empty
		removeInstanceAt(maybeCell);
	}
	else {
		// Alter instance, if build states change between others than empty
		removeInstanceAt(maybeCell);
		addInstanceAt(maybeCell, buildState);
	}
	maybeCell->updateBuildState(vbo_, buildState);
}