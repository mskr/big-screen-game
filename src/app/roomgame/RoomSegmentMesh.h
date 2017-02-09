#ifndef ROOM_SEGMENT_MESH_H
#define ROOM_SEGMENT_MESH_H
#include "core/gfx/mesh/MeshRenderable.h"
#include "InteractiveGrid.h"
#include "core/gfx/mesh/SceneMeshNode.h"
#include "core/gfx/mesh/SubMesh.h"
#include <glm/gtc/matrix_inverse.hpp>
#include "core/gfx/Material.h"
#include "core/gfx/Texture.h"

class RoomSegmentMesh : public viscom::MeshRenderable {
	int num_instances_;
	GLuint instance_buffer_;
	const size_t pool_allocation_bytes_;
	size_t num_reallocations_;
public:
	RoomSegmentMesh(viscom::Mesh* mesh, viscom::GPUProgram* program, size_t pool_allocation_bytes);
	void addInstance(glm::vec4 position, glm::vec3 scale, GridCell::BuildState type);
	void render(std::vector<GLint>* uniformLocations);
	void renderNode(std::vector<GLint>* uniformLocations, const viscom::SceneMeshNode* node, bool overrideBump=false);
	void renderSubMesh(std::vector<GLint>* uniformLocations, const glm::mat4& modelMatrix, const viscom::SubMesh* subMesh, bool overrideBump=false);
	struct InstanceAttribs {
		glm::vec3 translation;
		glm::vec3 scale;
		float zRotation;
		static const void setAttribPointer() {
			GLint transLoc = 3;
			GLint scaleLoc = 4;
			GLint zRotLoc = 5;
			glVertexAttribPointer(transLoc, 3, GL_FLOAT, false,
				sizeof(InstanceAttribs), (GLvoid*)0);
			glVertexAttribPointer(scaleLoc, 3, GL_FLOAT, false,
				sizeof(InstanceAttribs), (GLvoid*)(3 * sizeof(float)));
			glVertexAttribPointer(zRotLoc, 1, GL_FLOAT, false,
				sizeof(InstanceAttribs), (GLvoid*)(6 * sizeof(float)));
			glEnableVertexAttribArray(transLoc);
			glEnableVertexAttribArray(scaleLoc);
			glEnableVertexAttribArray(zRotLoc);
			glVertexAttribDivisor(transLoc, 1);
			glVertexAttribDivisor(scaleLoc, 1);
			glVertexAttribDivisor(zRotLoc, 1);
		}
	};
	typedef viscom::SimpleMeshVertex Vertex;
};

#endif