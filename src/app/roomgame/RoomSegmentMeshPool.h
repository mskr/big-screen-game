#ifndef ROOM_SEGMENT_MESH_POOL_H
#define ROOM_SEGMENT_MESH_POOL_H

#include <memory>
#include <stdexcept>
#include <functional>
#include "core/gfx/mesh/MeshRenderable.h"
#include "../Vertices.h"
#include "InteractiveGrid.h"
#include "RoomSegmentMesh.h"

class RoomSegmentMeshPool {
	// Map build state to multiple mesh variations
	// (when there is one mesh for multiple build states,
	// store the same pointer multiple times)
	std::unordered_map<GridCell::BuildState, std::vector<RoomSegmentMesh*>> meshes_;
	// Store build states contiguously for faster rendering
	// (only store one representative build state for one mesh)
	std::vector<GridCell::BuildState> render_list_;
	// Hold pointers to all meshes to control cleanup
	std::set<std::shared_ptr<viscom::Mesh>> owned_resources_;
	// The shader used by all meshes
	std::shared_ptr<viscom::GPUProgram> shader_;
	// Uniforms
	std::vector<GLint> matrix_uniform_locations_;
	std::vector<GLint> uniform_locations_;
	std::vector<std::function<void(GLint)>> uniform_callbacks_;
	GLint depth_pass_flag_uniform_location_;
public:
	RoomSegmentMeshPool(const size_t MAX_INSTANCES);
	~RoomSegmentMeshPool();
	// Init functions
	void loadShader(viscom::GPUProgramManager mgr);
	void addMesh(std::vector<GridCell::BuildState> types, std::shared_ptr<viscom::Mesh> mesh);
	void addMeshVariations(std::vector<GridCell::BuildState> types, std::vector<std::shared_ptr<viscom::Mesh>> mesh_variations);
	// Building function (request mesh for given build state)
	RoomSegmentMesh* getMeshOfType(GridCell::BuildState type);
	// Render function (renders each mesh once by using render list)
	void renderAllMeshes(glm::mat4& view_projection, GLint isDepthPass = 0);
	void renderAllMeshesExcept(glm::mat4& view_projection, GridCell::BuildState type_not_to_render, GLint isDepthPass = 0);
	void cleanup();
	// Set a uniform with a update function for the mesh pool shader
	void updateUniformEveryFrame(std::string uniform_name, std::function<void(GLint)> update_func);
	// Getter
	GLint getUniformLocation(size_t index);
	GLuint getShaderID();
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