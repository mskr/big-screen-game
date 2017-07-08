#ifndef ROOM_SEGMENT_MESH_H
#define ROOM_SEGMENT_MESH_H
#include "core/gfx/mesh/MeshRenderable.h"
#include "core/gfx/mesh/SceneMeshNode.h"
#include "core/gfx/mesh/SubMesh.h"

#include "GameMesh.h"
#include <glm/gtc/matrix_inverse.hpp>
#include "core/gfx/Material.h"
#include "core/gfx/Texture.h"
#include "../Vertices.h"

/* Class for instanced meshes.
 * Especially intended for segments of rooms on a grid.
 * However usable for all instanced meshes on the grid so far.
 * 
*/
class RoomSegmentMesh : public MeshBase<viscom::SimpleMeshVertex> {
public:
    /* Representation of an instance buffer supporting dynamic reallocations */
	struct InstanceBuffer {
		GLuint id_; // GL handle
		int num_instances_; // current size
		const size_t pool_allocation_bytes_; // chunk size for reallocations
		size_t num_reallocations_; // current number of reallocations
		InstanceBuffer(size_t pool_allocation_bytes); // constructor
	};
    /* Representation of a room segment mesh instance */
	struct Instance {
        // instance attributes (accessable from vertex shader)
		glm::vec3 translation = glm::vec3(0); // local translation
		GLfloat scale = 0.0f; // scale
		GLint buildState = 0; // build state (a value from GridCell::BuildState enum)
		GLint health = 0; // health points in [0, 100]
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
			glEnableVertexAttribArray(transLoc);
			glEnableVertexAttribArray(scaleLoc);
			glEnableVertexAttribArray(buildStateLoc);
			glEnableVertexAttribArray(healthLoc);
			size_t off = 0;
			glVertexAttribPointer(transLoc, 3, GL_FLOAT, false, sizeof(Instance), (GLvoid*)off);
			off += 3 * sizeof(GLfloat);
			glVertexAttribPointer(scaleLoc, 1, GL_FLOAT, false, sizeof(Instance), (GLvoid*)off);
			off += sizeof(GLfloat);
			glVertexAttribIPointer(buildStateLoc, 1, GL_INT, sizeof(Instance), (GLvoid*)off);
			off += sizeof(GLint);
			glVertexAttribIPointer(healthLoc, 1, GL_INT, sizeof(Instance), (GLvoid*)off);
			glVertexAttribDivisor(transLoc, 1);
			glVertexAttribDivisor(scaleLoc, 1);
			glVertexAttribDivisor(buildStateLoc, 1);
			glVertexAttribDivisor(healthLoc, 1);
		}
	};
    /* Representation of a range in an instance buffer */
	struct InstanceBufferRange {
		RoomSegmentMesh* mesh_ = 0; // instanced mesh reference
		InstanceBuffer* buffer_ = 0; // buffer reference
		int offset_instances_ = -1; // offset in buffer in units of instances
		int num_instances_ = -1; // instances in the range
	};
    /* Queue data structure for managing holes in buffers */
	struct NextFreeOffsetQueueElem {
		int offset_; // offset of the hole
		NextFreeOffsetQueueElem* el_; // next queue element
		NextFreeOffsetQueueElem(int off) : offset_(off), el_(0) {} // constructor
	};
private:
	InstanceBuffer room_ordered_buffer_; // instance buffer for finished room segments
	InstanceBuffer unordered_buffer_; // instance buffer for room segments in the building process
	NextFreeOffsetQueueElem* next_free_offset_; // beginning of the hole offset queue
	NextFreeOffsetQueueElem* last_free_offset_; // end of the hole offset queue
public:
	RoomSegmentMesh(viscom::Mesh* mesh, viscom::GPUProgram* program, size_t pool_allocation_bytes);
	~RoomSegmentMesh();
	InstanceBufferRange addInstanceUnordered(Instance);
	void removeInstanceUnordered(int offset_instances);
	InstanceBufferRange moveInstancesToRoomOrderedBuffer(std::initializer_list<int> offsets);
	void renderAllInstances(std::function<void(void)> uniformSetter, const glm::mat4& view_projection, GLint isDebugMode);
};

#endif