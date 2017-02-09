#include "RoomSegmentMeshPool.h"

RoomSegmentMeshPool::RoomSegmentMeshPool(InteractiveGrid* grid) :
	pool_allocation_bytes_corners_((grid->getNumCells() / 128 + 1) * sizeof(RoomSegmentMesh::InstanceAttribs)),
	pool_allocation_bytes_walls_((grid->getNumCells() / 32 + 1) * sizeof(RoomSegmentMesh::InstanceAttribs)),
	pool_allocation_bytes_floors_((grid->getNumCells() / 16 + 1) * sizeof(RoomSegmentMesh::InstanceAttribs))
{
	grid_ = grid;
	grid_->setRoomSegmentMeshPool(this);
	shader_ = 0;
}

void RoomSegmentMeshPool::setShader(std::shared_ptr<viscom::GPUProgram> shader) {
	shader_ = shader;
	uniform_locations_ = shader_->getUniformLocations({
		"viewProjectionMatrix", "subMeshLocalMatrix", "normalMatrix" });
}

void RoomSegmentMeshPool::addCornerMesh(std::shared_ptr<viscom::Mesh> mesh) {
	corners_.push_back(RoomSegmentMesh(mesh.get(), shader_.get(), pool_allocation_bytes_corners_));
	meshes_.push_back(mesh);
}

void RoomSegmentMeshPool::addWallMesh(std::shared_ptr<viscom::Mesh> mesh) {
	walls_.push_back(RoomSegmentMesh(mesh.get(), shader_.get(), pool_allocation_bytes_walls_));
	meshes_.push_back(mesh);
}

void RoomSegmentMeshPool::addFloorMesh(std::shared_ptr<viscom::Mesh> mesh) {
	floors_.push_back(RoomSegmentMesh(mesh.get(), shader_.get(), pool_allocation_bytes_floors_));
	meshes_.push_back(mesh);
}

RoomSegmentMesh* RoomSegmentMeshPool::getMeshOfType(GridCell::BuildState type) {
	return &corners_[0]; //TODO
}

void RoomSegmentMeshPool::render(glm::mat4 sgctMVP) {
	if (shader_ == 0)
		return;
	glUseProgram(shader_->getProgramId());
	glUniformMatrix4fv(uniform_locations_[0], 1, GL_FALSE, glm::value_ptr(sgctMVP));
	for (RoomSegmentMesh& segment : corners_)
		segment.render(&uniform_locations_);
}