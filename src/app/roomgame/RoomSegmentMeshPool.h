#ifndef ROOM_SEGMENT_MESH_POOL_H
#define ROOM_SEGMENT_MESH_POOL_H

#include <memory>
#include <stdexcept>
#include "core/gfx/mesh/MeshRenderable.h"
#include "../Vertices.h"
#include "InteractiveGrid.h"
#include "RoomSegmentMesh.h"

class RoomSegmentMeshPool {
	InteractiveGrid* grid_;
	// Map build state to multiple mesh variations
	std::unordered_map<GridCell::BuildState, std::vector<RoomSegmentMesh*>> meshes_;
	// Store build states contiguously for faster rendering
	std::vector<GridCell::BuildState> build_states_;
	// Hold pointers to all meshes to control cleanup
	std::set<std::shared_ptr<viscom::Mesh>> owned_resources_;
	// The shader used by all meshes
	std::shared_ptr<viscom::GPUProgram> shader_;
	std::vector<GLint> uniform_locations_;
public:
	RoomSegmentMeshPool(InteractiveGrid* grid);
	~RoomSegmentMeshPool();
	void setShader(std::shared_ptr<viscom::GPUProgram> shader);
	void addMesh(std::vector<GridCell::BuildState> types, std::shared_ptr<viscom::Mesh> mesh);
	void addMeshVariations(std::vector<GridCell::BuildState> types, std::vector<std::shared_ptr<viscom::Mesh>> mesh_variations);
	RoomSegmentMesh* getMeshOfType(GridCell::BuildState type);
	RoomSegmentMesh::InstanceBufferRange addInstanceUnordered(GridCell::BuildState type, RoomSegmentMesh::Instance instance);
	void renderAllMeshes(glm::mat4 view_projection);
	void cleanup();
private:
	// Pool allocation bytes based on estimated number of instances
	const size_t POOL_ALLOC_BYTES_CORNERS;
	const size_t POOL_ALLOC_BYTES_WALLS;
	const size_t POOL_ALLOC_BYTES_FLOORS;
	const size_t POOL_ALLOC_BYTES_OUTER_INFLUENCE;
	const size_t POOL_ALLOC_BYTES_DEFAULT;
	size_t determinePoolAllocationBytes(GridCell::BuildState type);
};

#endif