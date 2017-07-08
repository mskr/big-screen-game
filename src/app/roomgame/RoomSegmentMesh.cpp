#include "RoomSegmentMesh.h"

RoomSegmentMesh::InstanceBuffer::InstanceBuffer(size_t pool_allocation_bytes) :
	pool_allocation_bytes_(pool_allocation_bytes),
	num_instances_(0),
	num_reallocations_(1)
{
	glGenBuffers(1, &id_);
	glBindBuffer(GL_ARRAY_BUFFER, id_);
	glBufferData(GL_ARRAY_BUFFER, pool_allocation_bytes_, (GLvoid*)0, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

RoomSegmentMesh::RoomSegmentMesh(viscom::Mesh* mesh, viscom::GPUProgram* program, size_t pool_allocation_bytes) :
	MeshBase(mesh, program),
	room_ordered_buffer_(pool_allocation_bytes),
	unordered_buffer_(pool_allocation_bytes),
	next_free_offset_(0), last_free_offset_(0)
{
	// Connect instance buffers
	glBindVertexArray(vao_);
	//glBindBuffer(GL_ARRAY_BUFFER, room_ordered_buffer_.id_);
	//Instance::setAttribPointer();
	glBindBuffer(GL_ARRAY_BUFFER, unordered_buffer_.id_);
	Instance::setAttribPointer();
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

RoomSegmentMesh::~RoomSegmentMesh() {
	glDeleteBuffers(1, &room_ordered_buffer_.id_);
	glDeleteBuffers(1, &unordered_buffer_.id_);
	while (next_free_offset_) {
		NextFreeOffsetQueueElem* tmp = next_free_offset_->el_;
		delete next_free_offset_;
		next_free_offset_ = tmp;
	}
}

RoomSegmentMesh::InstanceBufferRange RoomSegmentMesh::addInstanceUnordered(Instance i) {
	int next_free_offset;
	if (!next_free_offset_) next_free_offset = unordered_buffer_.num_instances_;
	else {
		next_free_offset = next_free_offset_->offset_;
		NextFreeOffsetQueueElem* tmp = next_free_offset_->el_;
		delete next_free_offset_;
		next_free_offset_ = tmp;
	}
	if ((next_free_offset + 1) * sizeof(Instance) > unordered_buffer_.pool_allocation_bytes_ * unordered_buffer_.num_reallocations_) {
		// Realloc and copy
		glBindBuffer(GL_COPY_READ_BUFFER, unordered_buffer_.id_);
		GLuint tmpBuffer;
		glGenBuffers(1, &tmpBuffer);
		glBindBuffer(GL_COPY_WRITE_BUFFER, tmpBuffer);
		unordered_buffer_.num_reallocations_++;
		glBufferData(GL_COPY_WRITE_BUFFER, unordered_buffer_.pool_allocation_bytes_ * unordered_buffer_.num_reallocations_, 0, GL_STATIC_COPY);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, unordered_buffer_.num_instances_ * sizeof(Instance));
		// THIS IS WHAT MAKES REALLOCATION APPROACH ***UGLY***:
		glDeleteBuffers(1, &unordered_buffer_.id_); // Delete old instance buffer
		glDeleteVertexArrays(1, &vao_); // Delete old VAO
		resetShader(); // Create new VAO and connect old vertex buffer
		// Connect new instance buffer
		unordered_buffer_.id_ = tmpBuffer;
		glBindVertexArray(vao_);
		glBindBuffer(GL_ARRAY_BUFFER, unordered_buffer_.id_);
		Instance::setAttribPointer();
	}
	glBindBuffer(GL_ARRAY_BUFFER, unordered_buffer_.id_);
	glBufferSubData(GL_ARRAY_BUFFER, next_free_offset * sizeof(Instance), sizeof(Instance), &i);
	InstanceBufferRange r;
	r.buffer_ = &unordered_buffer_;
	r.mesh_ = this;
	r.num_instances_ = 1;
	r.offset_instances_ = next_free_offset;
	if (next_free_offset == unordered_buffer_.num_instances_)
		unordered_buffer_.num_instances_++;
	return r;
}

void RoomSegmentMesh::removeInstanceUnordered(int offset_instances) {
    // removing last offset in buffer is done by simply decrementing instance count
	if (offset_instances == unordered_buffer_.num_instances_ - 1) {
		unordered_buffer_.num_instances_--;
		return;
	}
    // removing instance from the middle of the buffer creates a hole
	if (!next_free_offset_) { // create the first hole
		next_free_offset_ = new NextFreeOffsetQueueElem(offset_instances);
		last_free_offset_ = next_free_offset_;
	}
	else { // append a hole to the queue
		last_free_offset_->el_ = new NextFreeOffsetQueueElem(offset_instances);
		last_free_offset_ = last_free_offset_->el_;
	}
	// cannot prevent GL from rendering holes too: insert a zero scaled instance into the hole (bad hack)
	Instance zeroScaledInstance;
	glBindBuffer(GL_ARRAY_BUFFER, unordered_buffer_.id_);
	glBufferSubData(GL_ARRAY_BUFFER, offset_instances * sizeof(Instance), sizeof(Instance), &zeroScaledInstance);
	
}

RoomSegmentMesh::InstanceBufferRange RoomSegmentMesh::moveInstancesToRoomOrderedBuffer(std::initializer_list<int> offsets) {
	//TODO copy and remove instances at given offsets
	return InstanceBufferRange();
}



void RoomSegmentMesh::renderAllInstances(std::function<void(void)> uniformSetter, const glm::mat4& view_projection, GLint isDebugMode) {
	//TODO Seperately draw unordered and ordered buffers
	if (unordered_buffer_.num_instances_ == 0) return;
	MeshBase::renderInstanced(uniformSetter, view_projection, unordered_buffer_.num_instances_, isDebugMode);
}