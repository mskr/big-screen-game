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
	for (GridCell::BuildState i : render_list_)
		for (RoomSegmentMesh* mesh : meshes_[i])
			delete mesh;
	owned_resources_.clear();
}

void RoomSegmentMeshPool::addMesh(std::vector<GridCell::BuildState> types, std::shared_ptr<viscom::Mesh> mesh) {
	// Map one mesh to possibly multiple build states
	// (first build state is considered representative for the mesh)
	size_t pool_allocation_bytes = determinePoolAllocationBytes(types[0]); // use pool alloc bytes of representative build state
	RoomSegmentMesh* meshptr = new RoomSegmentMesh(mesh.get(), shader_.get(), pool_allocation_bytes);
	for (GridCell::BuildState type : types) {
		// Copy the mesh pointer for each build state
		// (ensures that a mesh for a requested build state can quickly be found)
		meshes_[type].push_back(meshptr);
	}
	if (owned_resources_.find(mesh) == owned_resources_.end()) {
		// Store representative build state in render list
		// (ensures each mesh is only rendered once)
		render_list_.push_back(types[0]);
		// Hold the mesh resource in memory as long as the mesh pool lives
		owned_resources_.insert(mesh);
	}
}

void RoomSegmentMeshPool::addMeshVariations(std::vector<GridCell::BuildState> types, std::vector<std::shared_ptr<viscom::Mesh>> mesh_variations) {
	// Map possibly multiple meshes to possibly multiple build states
	// (if one of these build states is requested, a mesh is chosen randomly)
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
	for (GridCell::BuildState i : render_list_) {
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