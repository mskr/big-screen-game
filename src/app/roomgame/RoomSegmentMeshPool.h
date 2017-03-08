#ifndef ROOM_SEGMENT_MESH_POOL_H
#define ROOM_SEGMENT_MESH_POOL_H

#include <memory>
#include "core/gfx/mesh/MeshRenderable.h"
#include "../Vertices.h"
#include "InteractiveGrid.h"
#include "RoomSegmentMesh.h"

class RoomSegmentMeshPool {
	std::vector<RoomSegmentMesh*> corners_;
	std::vector<RoomSegmentMesh*> walls_;
	std::vector<RoomSegmentMesh*> floors_;
	std::vector<std::shared_ptr<viscom::Mesh>> meshes_;
	std::shared_ptr<viscom::GPUProgram> shader_;
	InteractiveGrid* grid_;
	const size_t pool_allocation_bytes_corners_;
	const size_t pool_allocation_bytes_walls_;
	const size_t pool_allocation_bytes_floors_;
	std::vector<GLint> uniform_locations_;
public:
	RoomSegmentMeshPool(InteractiveGrid* grid);
	~RoomSegmentMeshPool();
	void setShader(std::shared_ptr<viscom::GPUProgram> shader);
	void addCornerMesh(std::shared_ptr<viscom::Mesh> mesh);
	void addWallMesh(std::shared_ptr<viscom::Mesh> mesh);
	void addFloorMesh(std::shared_ptr<viscom::Mesh> mesh);
	RoomSegmentMesh* getMeshOfType(GridCell::BuildState type);
	RoomSegmentMesh::InstanceBufferRange addInstanceUnordered(GridCell::BuildState type, RoomSegmentMesh::Instance instance);
	void renderAllMeshes(glm::mat4 view_projection);
	void cleanup();
};

#endif