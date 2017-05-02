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
	viscom::MeshRenderable(mesh, Vertex::CreateVertexBuffer(mesh), program), // Fill vertex buffer
	room_ordered_buffer_(pool_allocation_bytes),
	unordered_buffer_(pool_allocation_bytes),
	next_free_offset_(0), last_free_offset_(0)
{
	// Create VAO and connect vertex buffer
	NotifyRecompiledShader<Vertex>(program);
	// Connect instance buffers
	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, room_ordered_buffer_.id_);
	Instance::setAttribPointer();
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
	/*if ((unordered_buffer_.num_instances_+1) * sizeof(Instance) > unordered_buffer_.pool_allocation_bytes_ * unordered_buffer_.num_reallocations_) {*/
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
		NotifyRecompiledShader<Vertex>(drawProgram_); // Create new VAO and connect old vertex buffer
		// Connect new instance buffer
		unordered_buffer_.id_ = tmpBuffer;
		glBindVertexArray(vao_);
		glBindBuffer(GL_ARRAY_BUFFER, unordered_buffer_.id_);
		Instance::setAttribPointer();
	}
	glBindBuffer(GL_ARRAY_BUFFER, unordered_buffer_.id_);
	glBufferSubData(GL_ARRAY_BUFFER, next_free_offset * sizeof(Instance), sizeof(Instance), &i);
	/*glBufferSubData(GL_ARRAY_BUFFER, unordered_buffer_.num_instances_ * sizeof(Instance), sizeof(Instance), &data);*/
	InstanceBufferRange r;
	r.buffer_ = &unordered_buffer_;
	r.mesh_ = this;
	r.num_instances_ = 1;
	r.offset_instances_ = next_free_offset;
	/*r.offset_instances_ = unordered_buffer_.num_instances_;*/
	if (next_free_offset == unordered_buffer_.num_instances_)
		unordered_buffer_.num_instances_++;
	return r;
}

void RoomSegmentMesh::removeInstanceUnordered(int offset_instances) {
	if (offset_instances == unordered_buffer_.num_instances_ - 1) {
		unordered_buffer_.num_instances_--;
		return;
	}
	if (!next_free_offset_) {
		next_free_offset_ = new NextFreeOffsetQueueElem(offset_instances);
		last_free_offset_ = next_free_offset_;
	}
	else {
		last_free_offset_->el_ = new NextFreeOffsetQueueElem(offset_instances);
		last_free_offset_ = last_free_offset_->el_;
	}
	Instance zeros;
	glBindBuffer(GL_ARRAY_BUFFER, unordered_buffer_.id_);
	glBufferSubData(GL_ARRAY_BUFFER, offset_instances * sizeof(Instance), sizeof(Instance), &zeros);
}

RoomSegmentMesh::InstanceBufferRange RoomSegmentMesh::moveInstancesToRoomOrderedBuffer(std::initializer_list<int> offsets) {
	//TODO copy and remove instances at given offsets
	return InstanceBufferRange();
}



void RoomSegmentMesh::renderAllInstances(std::vector<GLint>* uniformLocations) {
	if (unordered_buffer_.num_instances_ == 0) return; //TODO Seperately draw unordered and ordered buffers
	const viscom::SceneMeshNode* root = mesh_->GetRootNode();
	glBindVertexArray(vao_);
	renderNode(uniformLocations, root);
}

void RoomSegmentMesh::renderNode(std::vector<GLint>* uniformLocations, const viscom::SceneMeshNode* node, bool overrideBump) {
	auto localMatrix = node->GetLocalTransform();
	for (unsigned int i = 0; i < node->GetNumMeshes(); ++i)
		renderSubMesh(uniformLocations, localMatrix, node->GetMesh(i), overrideBump);
	for (unsigned int i = 0; i < node->GetNumNodes(); ++i)
		renderNode(uniformLocations, node->GetChild(i), overrideBump);
}

void RoomSegmentMesh::renderSubMesh(std::vector<GLint>* uniformLocations, const glm::mat4& modelMatrix, const viscom::SubMesh* subMesh, bool overrideBump) {
	if(uniformLocations->size() > 1)
		glUniformMatrix4fv(uniformLocations->at(1), 1, GL_FALSE, glm::value_ptr(modelMatrix));
	if(uniformLocations->size() > 2)
		glUniformMatrix3fv(uniformLocations->at(2), 1, GL_FALSE, glm::value_ptr(glm::inverseTranspose(glm::mat3(modelMatrix))));
	if (subMesh->GetMaterial()->diffuseTex && uniformLocations->size() > 2) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, subMesh->GetMaterial()->diffuseTex->getTextureId());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glUniform1i(uniformLocations_[2], 0);
	}
	if (subMesh->GetMaterial()->bumpTex && uniformLocations_.size() > 3) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, subMesh->GetMaterial()->bumpTex->getTextureId());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glUniform1i(uniformLocations_[3], 1);
		if (!overrideBump) glUniform1f(uniformLocations_[4], subMesh->GetMaterial()->bumpMultiplier);
	}
	//TODO Seperately draw unordered and ordered buffers
	glDrawElementsInstanced(GL_TRIANGLES, subMesh->GetNumberOfIndices(), GL_UNSIGNED_INT,
		(static_cast<char*> (nullptr)) + (subMesh->GetIndexOffset() * sizeof(unsigned int)), unordered_buffer_.num_instances_);
}