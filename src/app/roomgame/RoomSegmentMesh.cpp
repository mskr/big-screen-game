#include "RoomSegmentMesh.h"

RoomSegmentMesh::RoomSegmentMesh(viscom::Mesh* mesh, viscom::GPUProgram* program, size_t pool_allocation_bytes) :
	viscom::MeshRenderable(mesh, Vertex::CreateVertexBuffer(mesh), program),
	pool_allocation_bytes_(pool_allocation_bytes)
{
	num_instances_ = 0;
	this->NotifyRecompiledShader<Vertex>(program);
	glBindVertexArray(this->vao_);
	glGenBuffers(1, &instance_buffer_);
	glBindBuffer(GL_ARRAY_BUFFER, instance_buffer_);
	glBufferData(GL_ARRAY_BUFFER, pool_allocation_bytes_, (GLvoid*)0, GL_STATIC_DRAW);
	InstanceAttribs::setAttribPointer();
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	num_reallocations_ = 1;
}

void RoomSegmentMesh::addInstance(glm::vec4 position, glm::vec3 scale, GridCell::BuildState type) {
	if ((num_instances_+1) * sizeof(InstanceAttribs) > pool_allocation_bytes_ * num_reallocations_) {
		// Realloc
		glBindBuffer(GL_COPY_READ_BUFFER, instance_buffer_);
		GLuint tmpBuffer;
		glGenBuffers(1, &tmpBuffer);
		glBindBuffer(GL_COPY_WRITE_BUFFER, tmpBuffer);
		num_reallocations_++;
		glBufferData(GL_COPY_WRITE_BUFFER, pool_allocation_bytes_ * num_reallocations_, 0, GL_STATIC_DRAW);
		glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, num_instances_ * sizeof(InstanceAttribs));
		glDeleteBuffers(1, &instance_buffer_);
		instance_buffer_ = tmpBuffer;
	}
	glBindBuffer(GL_ARRAY_BUFFER, instance_buffer_);
	InstanceAttribs data;
	data.translation = glm::vec3(position);
	data.scale = scale;
	data.zRotation = 0.0f; //TODO decide rotation based on BuildState
	glBufferSubData(GL_ARRAY_BUFFER, num_instances_ * sizeof(InstanceAttribs), sizeof(InstanceAttribs), &data);
	num_instances_++;
}

void RoomSegmentMesh::renderSubMesh(std::vector<GLint>* uniformLocations, const glm::mat4& modelMatrix, const viscom::SubMesh* subMesh, bool overrideBump) {
	glUniformMatrix4fv(uniformLocations->at(1), 1, GL_FALSE,
		glm::value_ptr(modelMatrix));
	glUniformMatrix4fv(uniformLocations->at(2), 1, GL_FALSE,
		glm::value_ptr(glm::inverseTranspose(glm::mat3(modelMatrix))));
	if (subMesh->GetMaterial()->diffuseTex && uniformLocations->size() > 2) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, subMesh->GetMaterial()->diffuseTex->getTextureId());
		glUniform1i(uniformLocations_[2], 0);
	}
	if (subMesh->GetMaterial()->bumpTex && uniformLocations_.size() > 3) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, subMesh->GetMaterial()->bumpTex->getTextureId());
		glUniform1i(uniformLocations_[3], 1);
		if (!overrideBump) glUniform1f(uniformLocations_[4], subMesh->GetMaterial()->bumpMultiplier);
	}
	glDrawElementsInstanced(GL_TRIANGLES, subMesh->GetNumberOfIndices(), GL_UNSIGNED_INT,
		(static_cast<char*> (nullptr)) + (subMesh->GetIndexOffset() * sizeof(unsigned int)), num_instances_);
}

void RoomSegmentMesh::renderNode(std::vector<GLint>* uniformLocations, const viscom::SceneMeshNode* node, bool overrideBump) {
	auto localMatrix = node->GetLocalTransform();
	for (unsigned int i = 0; i < node->GetNumMeshes(); ++i)
		renderSubMesh(uniformLocations, localMatrix, node->GetMesh(i), overrideBump);
	for (unsigned int i = 0; i < node->GetNumNodes(); ++i)
		renderNode(uniformLocations, node->GetChild(i), overrideBump);
}

void RoomSegmentMesh::render(std::vector<GLint>* uniformLocations) {
	if (num_instances_ == 0) return;
	const viscom::SceneMeshNode* root = mesh_->GetRootNode();
	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	renderNode(uniformLocations, root);
}