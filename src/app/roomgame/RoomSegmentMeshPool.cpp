#include "RoomSegmentMeshPool.h"

RoomSegmentMeshPool::RoomSegmentMeshPool(InteractiveGrid* grid) :
	pool_allocation_bytes_corners_((grid->getNumCells() / 128 + 1) * sizeof(RoomSegmentMesh::Instance)),
	pool_allocation_bytes_walls_((grid->getNumCells() / 32 + 1) * sizeof(RoomSegmentMesh::Instance)),
	pool_allocation_bytes_floors_((grid->getNumCells() / 16 + 1) * sizeof(RoomSegmentMesh::Instance)),
	corners_{}, walls_{}, floors_{}
{
	grid_ = grid;
	grid_->setRoomSegmentMeshPool(this);
	shader_ = 0;
}

RoomSegmentMeshPool::~RoomSegmentMeshPool() {
}

void RoomSegmentMeshPool::cleanup() {
	for (RoomSegmentMesh* p : corners_) delete p;
	for (RoomSegmentMesh* p : walls_) delete p;
	for (RoomSegmentMesh* p : floors_) delete p;
}

void RoomSegmentMeshPool::setShader(std::shared_ptr<viscom::GPUProgram> shader) {
	shader_ = shader;
	uniform_locations_ = shader_->getUniformLocations({
		"viewProjectionMatrix", "subMeshLocalMatrix", "normalMatrix" });
}

void RoomSegmentMeshPool::addCornerMesh(std::shared_ptr<viscom::Mesh> mesh) {
	RoomSegmentMesh* m = new RoomSegmentMesh(mesh.get(), shader_.get(), pool_allocation_bytes_corners_);
	corners_.push_back(m);
	meshes_.push_back(mesh);
}

void RoomSegmentMeshPool::addWallMesh(std::shared_ptr<viscom::Mesh> mesh) {
	RoomSegmentMesh* m = new RoomSegmentMesh(mesh.get(), shader_.get(), pool_allocation_bytes_walls_);
	walls_.push_back(m);
	meshes_.push_back(mesh);
}

void RoomSegmentMeshPool::addFloorMesh(std::shared_ptr<viscom::Mesh> mesh) {
	RoomSegmentMesh* m = new RoomSegmentMesh(mesh.get(), shader_.get(), pool_allocation_bytes_floors_);
	floors_.push_back(m);
	meshes_.push_back(mesh);
}

RoomSegmentMesh* RoomSegmentMeshPool::getMeshOfType(GridCell::BuildState type) {
	srand((unsigned int)time(0));
	if (type == GridCell::BuildState::LEFT_LOWER_CORNER || type == GridCell::BuildState::RIGHT_LOWER_CORNER ||
		type == GridCell::BuildState::LEFT_UPPER_CORNER || type == GridCell::BuildState::RIGHT_UPPER_CORNER) {
		return corners_[rand() % corners_.size()];
	}
	else if (type == GridCell::BuildState::INSIDE_ROOM) {
		return floors_[rand() % floors_.size()];
	}
	else if (type == GridCell::BuildState::WALL_RIGHT || type == GridCell::BuildState::WALL_LEFT ||
		type == GridCell::BuildState::WALL_TOP || type == GridCell::BuildState::WALL_BOTTOM) {
		return walls_[rand() % walls_.size()];
	}
	else {
		// If the pool has no mesh for the requested build state...
		return corners_[0];
	}
}

RoomSegmentMesh::InstanceBufferRange RoomSegmentMeshPool::addInstanceUnordered(GridCell::BuildState type, RoomSegmentMesh::Instance instance) {
	RoomSegmentMesh* mesh = getMeshOfType(type);
	// Choose rotation for corners and walls
	if (type == GridCell::BuildState::LEFT_UPPER_CORNER || type == GridCell::BuildState::WALL_LEFT)
		instance.zRotation = glm::half_pi<float>();
	else if (type == GridCell::BuildState::LEFT_LOWER_CORNER || type == GridCell::BuildState::WALL_BOTTOM)
		instance.zRotation = 0.0f;
	else if (type == GridCell::BuildState::RIGHT_UPPER_CORNER || type == GridCell::BuildState::WALL_TOP)
		instance.zRotation = glm::pi<float>();
	else if (type == GridCell::BuildState::RIGHT_LOWER_CORNER || type == GridCell::BuildState::WALL_RIGHT)
		instance.zRotation = glm::three_over_two_pi<float>();
	return mesh->addInstanceUnordered(instance.translation, instance.scale, instance.zRotation);
}



void RoomSegmentMeshPool::renderAllMeshes(glm::mat4 view_projection) {
	if (shader_ == 0) return;
	glUseProgram(shader_->getProgramId());
	glUniformMatrix4fv(uniform_locations_[0], 1, GL_FALSE, glm::value_ptr(view_projection));
	//TODO Set other uniforms (normal matrix, submesh local matrix)
	for (RoomSegmentMesh* segment : corners_)
		segment->renderAllInstances(&uniform_locations_);
	for (RoomSegmentMesh* segment : walls_)
		segment->renderAllInstances(&uniform_locations_);
	for (RoomSegmentMesh* segment : floors_)
		segment->renderAllInstances(&uniform_locations_);
}