#ifndef GAME_MESH_H
#define GAME_MESH_H

#include <memory>
#include <iostream>

#include "glm\gtc/matrix_inverse.hpp"

#include "core/gfx/mesh/Mesh.h"
#include "core/gfx/GPUProgram.h"
#include "core/gfx/Material.h"
#include "core/gfx/Texture.h"
#include "core/gfx/mesh/SceneMeshNode.h"

#include "../Vertices.h"



template <class VERTEX_LAYOUT>
class MeshBase {

protected:

	GLuint vao_;
	GLuint vbo_;
	viscom::Mesh* mesh_;
	viscom::GPUProgram* program_;
	std::vector<GLint> uniformLocations_;


public:

	MeshBase(viscom::Mesh* mesh, viscom::GPUProgram* program) :
		mesh_(mesh), program_(program), vbo_(VERTEX_LAYOUT::CreateVertexBuffer(mesh)), vao_(0)
	{
		resetShader();
	}

	~MeshBase() {
		if (vbo_ != 0) glDeleteBuffers(1, &vbo_);
		vbo_ = 0;
		if (vao_ != 0) glDeleteVertexArrays(1, &vao_);
		vao_ = 0;
	}


protected:

	void resetShader() {
		glGenVertexArrays(1, &vao_);
		glBindVertexArray(vao_);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_->GetIndexBuffer());
		VERTEX_LAYOUT::SetVertexAttributes(program_);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		uniformLocations_ = program_->getUniformLocations({
			"subMeshLocalMatrix",
			"normalMatrix",
			"diffuseTexture",
			"bumpTexture",
			"bumpMultiplier",
			"viewProjectionMatrix",
			"isDebugMode"
		});
	}

	void render(const glm::mat4& vpMatrix, GLint isDebugMode = 0, const glm::mat4& modelMatrix = glm::mat4(1), bool overrideBump = false) const {
		glUseProgram(program_->getProgramId());
		glBindVertexArray(vao_);
		forEachSubmeshOf(mesh_->GetRootNode(), modelMatrix, [&](const viscom::SubMesh* submesh, const glm::mat4& localTransform) {
			bindUniformsAndTextures(vpMatrix, localTransform, submesh->GetMaterial()->diffuseTex, submesh->GetMaterial()->bumpTex, submesh->GetMaterial()->bumpMultiplier, overrideBump, isDebugMode);
			if (isDebugMode == 1) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDrawElements(GL_TRIANGLES, submesh->GetNumberOfIndices(), GL_UNSIGNED_INT,
				(static_cast<char*> (nullptr)) + (submesh->GetIndexOffset() * sizeof(unsigned int)));
		});
	}

	void render(std::function<void(void)> outsideUniformSetter, const glm::mat4& vpMatrix, GLint isDebugMode = 0, const glm::mat4& modelMatrix = glm::mat4(1), bool overrideBump = false) const {
		glUseProgram(program_->getProgramId());
		glBindVertexArray(vao_);
		forEachSubmeshOf(mesh_->GetRootNode(), modelMatrix, [&](const viscom::SubMesh* submesh, const glm::mat4& localTransform) {
			bindUniformsAndTextures(vpMatrix, localTransform, submesh->GetMaterial()->diffuseTex, submesh->GetMaterial()->bumpTex, submesh->GetMaterial()->bumpMultiplier, overrideBump, isDebugMode);
//			std::cout << "reached a" << std::endl;
			outsideUniformSetter();
//			std::cout << "reached b" << std::endl;
			if (isDebugMode == 1) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//			std::cout << "reached c" << std::endl;
			glDrawElements(GL_TRIANGLES, submesh->GetNumberOfIndices(), GL_UNSIGNED_INT,
				(static_cast<char*> (nullptr)) + (submesh->GetIndexOffset() * sizeof(unsigned int)));
//			std::cout << "reached d" << std::endl;
		});
	}

	void renderInstanced(const glm::mat4& vpMatrix, GLsizei num_instances, GLint isDebugMode = 0, const glm::mat4& modelMatrix = glm::mat4(1), bool overrideBump = false) const {
		glUseProgram(program_->getProgramId());
		glBindVertexArray(vao_);
		forEachSubmeshOf(mesh_->GetRootNode(), modelMatrix, [&](const viscom::SubMesh* submesh, const glm::mat4& localTransform) {
			bindUniformsAndTextures(vpMatrix, localTransform, submesh->GetMaterial()->diffuseTex, submesh->GetMaterial()->bumpTex, submesh->GetMaterial()->bumpMultiplier, overrideBump, isDebugMode);
			if (isDebugMode == 1) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDrawElementsInstanced(GL_TRIANGLES, submesh->GetNumberOfIndices(), GL_UNSIGNED_INT,
				(static_cast<char*> (nullptr)) + (submesh->GetIndexOffset() * sizeof(unsigned int)), num_instances);
		});
	}

	void renderInstanced(std::function<void(void)> outsideUniformSetter, const glm::mat4& vpMatrix, GLsizei num_instances, GLint isDebugMode = 0, const glm::mat4& modelMatrix = glm::mat4(1), bool overrideBump = false) const {
		glUseProgram(program_->getProgramId());
		glBindVertexArray(vao_);
		forEachSubmeshOf(mesh_->GetRootNode(), modelMatrix, [&](const viscom::SubMesh* submesh, const glm::mat4& localTransform) {
			bindUniformsAndTextures(vpMatrix, localTransform, submesh->GetMaterial()->diffuseTex, submesh->GetMaterial()->bumpTex, submesh->GetMaterial()->bumpMultiplier, overrideBump, isDebugMode);
			outsideUniformSetter();
			if (isDebugMode == 1) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDrawElementsInstanced(GL_TRIANGLES, submesh->GetNumberOfIndices(), GL_UNSIGNED_INT,
				(static_cast<char*> (nullptr)) + (submesh->GetIndexOffset() * sizeof(unsigned int)), num_instances);
		});
	}


private:

	void forEachSubmeshOf(const viscom::SceneMeshNode* subtree, const glm::mat4& transform, std::function<void(const viscom::SubMesh*, const glm::mat4&)> callback) const {
		auto localTransform = subtree->GetLocalTransform() * transform;
		unsigned int bla = subtree->GetNumMeshes();
		for (unsigned int i = 0; i < subtree->GetNumMeshes(); ++i)
			callback(subtree->GetMesh(i), localTransform);
		for (unsigned int i = 0; i < subtree->GetNumNodes(); ++i)
			forEachSubmeshOf(subtree->GetChild(i), localTransform, callback);
	}

	void bindUniformsAndTextures(const glm::mat4& vpMatrix, const glm::mat4& localMatrix,
			std::shared_ptr<const viscom::Texture> diffuseTex, std::shared_ptr<const viscom::Texture> bumpTex, GLfloat bumpMultiplier,
			bool overrideBump = false, GLint isDebugMode = 0) const {
		glUniformMatrix4fv(uniformLocations_[0], 1, GL_FALSE, glm::value_ptr(localMatrix));
		glUniformMatrix3fv(uniformLocations_[1], 1, GL_FALSE, glm::value_ptr(glm::inverseTranspose(glm::mat3(localMatrix))));
		if (diffuseTex && uniformLocations_.size() > 2) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, diffuseTex->getTextureId());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glUniform1i(uniformLocations_[2], 0);
		}
		if (bumpTex && uniformLocations_.size() > 3) {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, bumpTex->getTextureId());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glUniform1i(uniformLocations_[3], 1);
			if (!overrideBump) glUniform1f(uniformLocations_[4], bumpMultiplier);
		}
		glUniformMatrix4fv(uniformLocations_[5], 1, GL_FALSE, glm::value_ptr(vpMatrix));
		glUniform1i(uniformLocations_[6], isDebugMode);
	}
};





class SimpleGameMesh : public MeshBase<viscom::SimpleMeshVertex> {
protected:
	std::shared_ptr<viscom::Mesh> mesh_resource_;
	std::shared_ptr<viscom::GPUProgram> shader_resource_;
	glm::mat4 model_matrix_;
	GLint uloc_view_projection_;
	GLint uloc_debug_mode_flag_;
public:
	SimpleGameMesh(std::shared_ptr<viscom::Mesh> mesh, std::shared_ptr<viscom::GPUProgram> shader) :
		MeshBase(mesh.get(), shader.get()),
		mesh_resource_(mesh), shader_resource_(shader)
	{
		uloc_view_projection_ = shader->getUniformLocation("viewProjectionMatrix");
		uloc_debug_mode_flag_ = shader->getUniformLocation("isDebugMode");
	}
	void transform(glm::mat4& t) {
		model_matrix_ *= t;
	}
	virtual void render(glm::mat4& vp, GLint isDebugMode = 0) const {
		MeshBase::render(vp, isDebugMode, model_matrix_);
	}
};

class ShadowReceivingMesh : public SimpleGameMesh {
	GLint uloc_lightspace_matrix_;
	GLint uloc_shadow_map_;
public:
	ShadowReceivingMesh(std::shared_ptr<viscom::Mesh> mesh, std::shared_ptr<viscom::GPUProgram> shader);
	void render(glm::mat4& vp, glm::mat4& lightspace, GLuint shadowMap, GLint isDebugMode = 0) const;
};

#endif