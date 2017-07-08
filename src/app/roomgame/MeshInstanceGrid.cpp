#include "MeshInstanceGrid.h"

MeshInstanceGrid::MeshInstanceGrid(size_t columns, size_t rows, float height, RoomSegmentMeshPool* meshpool) :
	RoomInteractiveGrid(columns, rows, height), meshpool_(meshpool)
{

}
/* Called only on master //TODO
    Updates CPU data on master.
    This data is synchronized with slaves before each frame.
    After synchronization, each slave updates its GPU data.
    RoomSegmentMesh has two parts:
        1) Instances on CPU
        2) The after-sync method to upload all instances
*/
void MeshInstanceGrid::addInstanceAt(GridCell* c, GridCell::BuildState st) {
	RoomSegmentMesh* mesh = meshpool_->getMeshOfType(st);
	if (!mesh) return;
	RoomSegmentMesh::Instance instance;
	instance.scale = cell_size_ / 2.0f; // assume model extends [-1,1]^3
	instance.translation = translation_; // grid translation
	instance.translation += glm::vec3(c->getPosition(), 0.0f); // + relative cell translation
	instance.translation += glm::vec3(cell_size_ / 2.0f, -cell_size_ / 2.0f, 0.0f); // with origin in middle of cell
	instance.buildState = st;
	instance.health = c->getHealthPoints();
	c->setMeshInstance(mesh->addInstanceUnordered(instance));
}

/* Called only on master //TODO
    Same scheme as with addInstanceAt, see above
*/
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

void MeshInstanceGrid::onMeshpoolInitialized() {

}