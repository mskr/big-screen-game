#ifndef ROOM_SEGMENT_MESH_H
#define ROOM_SEGMENT_MESH_H
#include "core/gfx/mesh/MeshRenderable.h"
#include "core/gfx/mesh/SceneMeshNode.h"
#include "core/gfx/mesh/SubMesh.h"
#include <glm/gtc/matrix_inverse.hpp>
#include "core/gfx/Material.h"
#include "core/gfx/Texture.h"
#include "../Vertices.h"

class RoomSegmentMesh : public viscom::MeshRenderable {
public:
	typedef viscom::SimpleMeshVertex Vertex;
	struct InstanceBuffer {
		GLuint id_;
		int num_instances_;
		const size_t pool_allocation_bytes_;
		size_t num_reallocations_;
		InstanceBuffer(size_t pool_allocation_bytes);
	};
	struct Instance {
		glm::vec3 translation = glm::vec3(0);
		glm::vec3 scale = glm::vec3(0);
		float zRotation = 0.0f;
		static const void setAttribPointer() {
			GLint transLoc = 3;
			GLint scaleLoc = 4;
			GLint zRotLoc = 5;
			glVertexAttribPointer(transLoc, 3, GL_FLOAT, false,
				sizeof(Instance), (GLvoid*)0);
			glVertexAttribPointer(scaleLoc, 3, GL_FLOAT, false,
				sizeof(Instance), (GLvoid*)(3 * sizeof(float)));
			glVertexAttribPointer(zRotLoc, 1, GL_FLOAT, false,
				sizeof(Instance), (GLvoid*)(6 * sizeof(float)));
			glEnableVertexAttribArray(transLoc);
			glEnableVertexAttribArray(scaleLoc);
			glEnableVertexAttribArray(zRotLoc);
			glVertexAttribDivisor(transLoc, 1);
			glVertexAttribDivisor(scaleLoc, 1);
			glVertexAttribDivisor(zRotLoc, 1);
		}
	};
	struct InstanceBufferRange {
		RoomSegmentMesh* mesh_ = 0;
		InstanceBuffer* buffer_ = 0;
		int offset_instances_ = -1;
		int num_instances_ = -1;
	};
	struct NextFreeOffsetQueueElem {
		int offset_;
		NextFreeOffsetQueueElem* el_;
		NextFreeOffsetQueueElem(int off) :
			offset_(off), el_(0) {}
	};
private:
	InstanceBuffer room_ordered_buffer_;
	InstanceBuffer unordered_buffer_;
	NextFreeOffsetQueueElem* next_free_offset_;
	NextFreeOffsetQueueElem* last_free_offset_;
public:
	RoomSegmentMesh(viscom::Mesh* mesh, viscom::GPUProgram* program, size_t pool_allocation_bytes);
	~RoomSegmentMesh();
	InstanceBufferRange addInstanceUnordered(glm::vec3 position, glm::vec3 scale, float zRot);
	void removeInstanceUnordered(int offset_instances);
	InstanceBufferRange moveInstancesToRoomOrderedBuffer(std::initializer_list<int> offsets);
	void renderAllInstances(std::vector<GLint>* uniformLocations);
private:
	void renderNode(std::vector<GLint>* uniformLocations,
		const viscom::SceneMeshNode* node, bool overrideBump=false);
	void renderSubMesh(std::vector<GLint>* uniformLocations,
		const glm::mat4& modelMatrix, const viscom::SubMesh* subMesh, bool overrideBump=false);
};

#endif