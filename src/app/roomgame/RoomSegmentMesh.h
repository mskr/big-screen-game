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
		GLfloat zRotation = 0.0f;
		GLint buildState0 = 0; // last build state
		GLint buildState1 = 0; // current build state
		// Same for neighbor build states
		//TODO Currently, this information is only reliable for outer influence instances
		// Room segment instances are not updated often enough because information isnt needed
		glm::ivec2 neighborBuildStatesPacked0 = glm::ivec2(0);
		glm::ivec2 neighborBuildStatesPacked1 = glm::ivec2(0);
		GLint health = 0;
		static const void updateBuildState(GLuint buffer, int offset_instances, int st) {
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glCopyBufferSubData(GL_ARRAY_BUFFER, GL_ARRAY_BUFFER,
				(offset_instances + 1) * sizeof(Instance) - sizeof(health) - 2 * sizeof(glm::ivec2) - sizeof(GLint),
				(offset_instances + 1) * sizeof(Instance) - sizeof(health) - 2 * sizeof(glm::ivec2) - 2 * sizeof(GLint),
				sizeof(GLint));
			glBufferSubData(GL_ARRAY_BUFFER,
				(offset_instances + 1) * sizeof(Instance) - sizeof(health) - 2 * sizeof(glm::ivec2) - sizeof(GLint),
				sizeof(GLint), &st);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		static const void updateNeighborBuildStatesPacked(GLuint buffer, int offset_instances, glm::ivec2 v) {
			glBindBuffer(GL_ARRAY_BUFFER, buffer);
			glCopyBufferSubData(GL_ARRAY_BUFFER, GL_ARRAY_BUFFER,
				(offset_instances + 1) * sizeof(Instance) - sizeof(health) - sizeof(glm::ivec2),
				(offset_instances + 1) * sizeof(Instance) - sizeof(health) - 2 * sizeof(glm::ivec2),
				sizeof(glm::ivec2));
			glBufferSubData(GL_ARRAY_BUFFER,
				(offset_instances + 1) * sizeof(Instance) - sizeof(health) - sizeof(glm::ivec2),
				sizeof(glm::ivec2), glm::value_ptr(v));
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
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
			GLint zRotLoc = 5;
			GLint buildStateLoc0 = 6;
			GLint buildStateLoc1 = 7;
			GLint nborStatesLoc0 = 8;
			GLint nborStatesLoc1 = 9;
			GLint healthLoc = 10;
			size_t offset = 0;
			glVertexAttribPointer(transLoc, 3, GL_FLOAT, false,
				sizeof(Instance), (GLvoid*)offset);
			offset += 3 * sizeof(GLfloat);
			glVertexAttribPointer(scaleLoc, 3, GL_FLOAT, false,
				sizeof(Instance), (GLvoid*)offset);
			offset += 3 * sizeof(GLfloat);
			glVertexAttribPointer(zRotLoc, 1, GL_FLOAT, false,
				sizeof(Instance), (GLvoid*)offset);
			offset += sizeof(GLfloat);
			glVertexAttribIPointer(buildStateLoc0, 1, GL_INT,
				sizeof(Instance), (GLvoid*)offset);
			offset += sizeof(GLint);
			glVertexAttribIPointer(buildStateLoc1, 1, GL_INT,
				sizeof(Instance), (GLvoid*)offset);
			offset += sizeof(GLint);
			glVertexAttribIPointer(nborStatesLoc0, 2, GL_INT,
				sizeof(Instance), (GLvoid*)offset);
			offset += 2 * sizeof(GLint);
			glVertexAttribIPointer(nborStatesLoc1, 2, GL_INT,
				sizeof(Instance), (GLvoid*)offset);
			offset += 2 * sizeof(GLint);
			glVertexAttribIPointer(healthLoc, 1, GL_INT,
				sizeof(Instance), (GLvoid*)offset);
			glEnableVertexAttribArray(transLoc);
			glEnableVertexAttribArray(scaleLoc);
			glEnableVertexAttribArray(zRotLoc);
			glEnableVertexAttribArray(buildStateLoc0);
			glEnableVertexAttribArray(buildStateLoc1);
			glEnableVertexAttribArray(nborStatesLoc0);
			glEnableVertexAttribArray(nborStatesLoc1);
			glEnableVertexAttribArray(healthLoc);
			glVertexAttribDivisor(transLoc, 1);
			glVertexAttribDivisor(scaleLoc, 1);
			glVertexAttribDivisor(zRotLoc, 1);
			glVertexAttribDivisor(buildStateLoc0, 1);
			glVertexAttribDivisor(buildStateLoc1, 1);
			glVertexAttribDivisor(nborStatesLoc0, 1);
			glVertexAttribDivisor(nborStatesLoc1, 1);
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