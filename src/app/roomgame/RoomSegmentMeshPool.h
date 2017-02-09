#ifndef ROOM_SEGMENT_MESH_POOL_H
#define ROOM_SEGMENT_MESH_POOL_H

#include <memory>
#include "core/gfx/mesh/MeshRenderable.h"
#include "../Vertices.h"
#include "InteractiveGrid.h"

class RoomSegmentMesh;
#include "RoomSegmentMesh.h"

class RoomSegmentMeshPool {
	std::vector<RoomSegmentMesh> corners_;
	std::vector<RoomSegmentMesh> walls_;
	std::vector<RoomSegmentMesh> floors_;
	std::vector<std::shared_ptr<viscom::Mesh>> meshes_;
	std::shared_ptr<viscom::GPUProgram> shader_;
	InteractiveGrid* grid_;
	const size_t pool_allocation_bytes_corners_;
	const size_t pool_allocation_bytes_walls_;
	const size_t pool_allocation_bytes_floors_;
	std::vector<GLint> uniform_locations_;
public:
	RoomSegmentMeshPool(InteractiveGrid* grid);
	void setShader(std::shared_ptr<viscom::GPUProgram> shader);
	void addCornerMesh(std::shared_ptr<viscom::Mesh> mesh);
	void addWallMesh(std::shared_ptr<viscom::Mesh> mesh);
	void addFloorMesh(std::shared_ptr<viscom::Mesh> mesh);
	RoomSegmentMesh* getMeshOfType(GridCell::BuildState type);
	void render(glm::mat4 sgctMVP);
};

#endif