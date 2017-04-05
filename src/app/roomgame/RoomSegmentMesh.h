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
	typedef viscom::SimpleMeshVertex Vertex; // vertex attribs
	struct InstanceBuffer {
		GLuint id_;
		int num_instances_;
		const size_t pool_allocation_bytes_;
		size_t num_reallocations_;
		InstanceBuffer(size_t pool_allocation_bytes);
	};
	struct Instance { // instance attribs
		glm::vec3 translation = glm::vec3(0);
		GLfloat scale = 0.0f;
		GLint buildState = 0;
		GLint health = 0;
		static const void updateHealth(GLuint buffer, int offset_instances, int h) {
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glBufferSubData(GL_ARRAY_BUFFER,
				(offset_instances + 1) * sizeof(Instance) - sizeof(health),
				sizeof(health), &h);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		static const void setAttribPointer() {
			GLint transLoc = 3;
			GLint scaleLoc = 4;
			GLint buildStateLoc = 5;
			GLint healthLoc = 6;
			size_t off = 0;
			glVertexAttribPointer(transLoc, 3, GL_FLOAT, false, sizeof(Instance), (GLvoid*)off);
			off += 3 * sizeof(GLfloat);
			glVertexAttribPointer(scaleLoc, 1, GL_FLOAT, false, sizeof(Instance), (GLvoid*)off);
			off += sizeof(GLfloat);
			glVertexAttribIPointer(buildStateLoc, 1, GL_INT, sizeof(Instance), (GLvoid*)off);
			off += sizeof(GLint);
			glVertexAttribIPointer(healthLoc, 1, GL_INT, sizeof(Instance), (GLvoid*)off);
			glEnableVertexAttribArray(transLoc);
			glEnableVertexAttribArray(scaleLoc);
			glEnableVertexAttribArray(buildStateLoc);
			glEnableVertexAttribArray(healthLoc);
			glVertexAttribDivisor(transLoc, 1);
			glVertexAttribDivisor(scaleLoc, 1);
			glVertexAttribDivisor(buildStateLoc, 1);
			glVertexAttribDivisor(healthLoc, 1);
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
	InstanceBufferRange addInstanceUnordered(Instance);
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