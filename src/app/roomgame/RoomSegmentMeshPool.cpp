#include "RoomSegmentMeshPool.h"

RoomSegmentMeshPool::RoomSegmentMeshPool(const size_t MAX_INSTANCES) :
	POOL_ALLOC_BYTES_CORNERS((MAX_INSTANCES / 128 + 1) * sizeof(RoomSegmentMesh::Instance)),
	POOL_ALLOC_BYTES_WALLS((MAX_INSTANCES / 32 + 1) * sizeof(RoomSegmentMesh::Instance)),
	POOL_ALLOC_BYTES_FLOORS((MAX_INSTANCES / 16 + 1) * sizeof(RoomSegmentMesh::Instance)),
	POOL_ALLOC_BYTES_OUTER_INFLUENCE((MAX_INSTANCES / 16 + 1) * sizeof(RoomSegmentMesh::Instance)),
	POOL_ALLOC_BYTES_DEFAULT(MAX_INSTANCES * sizeof(RoomSegmentMesh::Instance))
{
	shader_ = 0;
}

RoomSegmentMeshPool::~RoomSegmentMeshPool() {
}

void RoomSegmentMeshPool::cleanup() {
	// Delete all pointers to RoomSegmentMeshes, but no pointer twice
	for (GridCell::BuildState i : build_states_) {
		for (RoomSegmentMesh* m : meshes_[i]) {
			if (m) {
				delete m;
				// Null all pointers to m after deletion of m
				for (GridCell::BuildState j : build_states_) {
					for (unsigned int k = 0; k < meshes_[j].size(); k++) {
						if (m == meshes_[j][k]) meshes_[j][k] = 0;
					}
				}
			}
		}
	}
	owned_resources_.clear();
}

size_t RoomSegmentMeshPool::determinePoolAllocationBytes(GridCell::BuildState type) {
	if (type == GridCell::BuildState::LEFT_LOWER_CORNER || type == GridCell::BuildState::RIGHT_LOWER_CORNER ||
		type == GridCell::BuildState::LEFT_UPPER_CORNER || type == GridCell::BuildState::RIGHT_UPPER_CORNER) {
		return POOL_ALLOC_BYTES_CORNERS;
	}
	else if (type == GridCell::BuildState::INSIDE_ROOM) {
		return POOL_ALLOC_BYTES_FLOORS;
	}
	else if (type == GridCell::BuildState::WALL_RIGHT || type == GridCell::BuildState::WALL_LEFT ||
		type == GridCell::BuildState::WALL_TOP || type == GridCell::BuildState::WALL_BOTTOM) {
		return POOL_ALLOC_BYTES_WALLS;
	}
	else if (type == GridCell::BuildState::OUTER_INFLUENCE) {
		return POOL_ALLOC_BYTES_OUTER_INFLUENCE;
	}
	else {
		return POOL_ALLOC_BYTES_DEFAULT;
	}
}

void RoomSegmentMeshPool::addMesh(std::vector<GridCell::BuildState> types, std::shared_ptr<viscom::Mesh> mesh) {
	size_t pool_allocation_bytes = determinePoolAllocationBytes(types[0]);
	RoomSegmentMesh* segmentMesh = new RoomSegmentMesh(mesh.get(), shader_.get(), pool_allocation_bytes);
	for (GridCell::BuildState type : types) {
		meshes_[type].push_back(segmentMesh);
		build_states_.insert(type);
	}
	owned_resources_.insert(mesh);
}

void RoomSegmentMeshPool::addMeshVariations(std::vector<GridCell::BuildState> types, std::vector<std::shared_ptr<viscom::Mesh>> mesh_variations) {
	for (std::shared_ptr<viscom::Mesh> variation : mesh_variations)
		addMesh(types, variation);
}

RoomSegmentMesh* RoomSegmentMeshPool::getMeshOfType(GridCell::BuildState type) {
	std::vector<RoomSegmentMesh*> mesh_variations;
	try {
		mesh_variations = meshes_.at(type);
	}
	catch (std::out_of_range) {
		// If the pool does not have the requested type
		printf("Pool has no mesh for type %d\n", (int)type);
		return 0;
	}
	srand((unsigned int)time(0));
	unsigned int variation = rand() % mesh_variations.size();
	return mesh_variations[variation];
}



void RoomSegmentMeshPool::renderAllMeshes(glm::mat4 view_projection) {
	glUseProgram(shader_->getProgramId());
	glUniformMatrix4fv(matrix_uniform_locations_[0], 1, GL_FALSE, glm::value_ptr(view_projection));
	for (unsigned int i = 0; i < uniform_locations_.size(); i++) uniform_callbacks_[i](uniform_locations_[i]);
	for (GridCell::BuildState i : build_states_) {
		for (RoomSegmentMesh* mesh : meshes_[i]) {
			mesh->renderAllInstances(&matrix_uniform_locations_);
		}
	}
}

void RoomSegmentMeshPool::loadShader(viscom::GPUProgramManager mgr) {
	shader_ = mgr.GetResource("foregroundMesh",
			std::initializer_list<std::string>{ "foregroundMesh.vert", "foregroundMesh.frag" });
	matrix_uniform_locations_ = shader_->getUniformLocations({
		"viewProjectionMatrix", "subMeshLocalMatrix", "normalMatrix" });
}

void RoomSegmentMeshPool::updateUniformEveryFrame(std::string uniform_name, std::function<void(GLint)> update_func) {
	uniform_locations_.push_back(shader_->getUniformLocation(uniform_name));
	uniform_callbacks_.push_back(update_func);
}

GLint RoomSegmentMeshPool::getUniformLocation(size_t index) {
	return uniform_locations_[index];
}

GLuint RoomSegmentMeshPool::getShaderID() {
	return shader_->getProgramId();
}