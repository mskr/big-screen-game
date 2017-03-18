#include "MeshInstanceGrid.h"

MeshInstanceGrid::MeshInstanceGrid(size_t columns, size_t rows, float height, RoomSegmentMeshPool* meshpool) :
	RoomInteractiveGrid(columns, rows, height), meshpool_(meshpool)
{

}

void MeshInstanceGrid::buildAt(size_t col, size_t row, GridCell::BuildState buildState) {
	GridCell* maybeCell = getCellAt(col, row);
	if (!maybeCell) return;
	if (maybeCell->getBuildState() == buildState) return;
	RoomSegmentMesh::InstanceBufferRange bufferRange;
	if (maybeCell->getBuildState() == GridCell::BuildState::EMPTY) {
		// Add instance, if cell was empty
		RoomSegmentMesh::Instance instance;
		instance.scale = glm::vec3(model_matrix_ * glm::vec4(cell_size_)) / 2.0f;
		instance.translation = glm::vec3(model_matrix_ * 
			glm::vec4(maybeCell->getPosition() + glm::vec2(cell_size_/2.0f, -cell_size_/2.0f), 0.0f, 1.0f));
		bufferRange = meshpool_->addInstanceUnordered(buildState, instance);
		maybeCell->setMeshInstance(bufferRange);
	}
	else if (buildState == GridCell::BuildState::EMPTY) {
		// Remove instance, if cell is going to be empty
		bufferRange = maybeCell->getMeshInstance();
		if(bufferRange.mesh_)
			bufferRange.mesh_->removeInstanceUnordered(bufferRange.offset_instances_);
	}
	else {
		// Alter instance, if build states change between others than empty
		bufferRange = maybeCell->getMeshInstance();
		if (bufferRange.mesh_)
			bufferRange.mesh_->removeInstanceUnordered(bufferRange.offset_instances_);
		RoomSegmentMesh::Instance instance;
		instance.scale = glm::vec3(model_matrix_ * glm::vec4(cell_size_)) / 2.0f;
		instance.translation = glm::vec3(model_matrix_ * 
			glm::vec4(maybeCell->getPosition() + glm::vec2(cell_size_/2.0f, -cell_size_/2.0f), 0.0f, 1.0f));
		bufferRange = meshpool_->addInstanceUnordered(buildState, instance);
		maybeCell->setMeshInstance(bufferRange);
	}
	maybeCell->updateBuildState(vbo_, buildState);
}