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
#include "sgct.h"

/* Base class for all meshes rendered by the roomgame.
 * Construct with a vertex class as template parameter (providing CreateVertexBuffer and SetVertexAttributes functions).
 * Uses naked pointers to mesh and shader (according resources should be owned by extending classes).
 * Holds VAO, VBO and the shader locations of common uniform variables.
 * Common uniforms that are provided:
	"subMeshLocalMatrix",
	"normalMatrix",
	"diffuseTexture",
	"bumpTexture",
	"bumpMultiplier",
	"viewProjectionMatrix",
	"isDebugMode"
 * Provides instanced and non-instanced render functions.
 * Both functions have one overload which takes a callback where custom uniform variables can be set.
 * Custom uniform locations and updates are managed by extending classes.
 * Works with meshes containing a hierarchy of scene mesh nodes (as imported by assimp).
*/
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
			"isDebugMode",
            "time"
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
			outsideUniformSetter();
			if (isDebugMode == 1) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glDrawElements(GL_TRIANGLES, submesh->GetNumberOfIndices(), GL_UNSIGNED_INT,
				(static_cast<char*> (nullptr)) + (submesh->GetIndexOffset() * sizeof(unsigned int)));
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
        glUniform1f(uniformLocations_[7], (float)glfwGetTime());
	}
};




/* Simple mesh class extending MeshBase.
* Owns mesh and shader resources.
* Adds a model matrix for dynamic transformation.
*/
class SimpleGameMesh : public MeshBase<viscom::SimpleMeshVertex> {
protected:
	std::shared_ptr<viscom::Mesh> mesh_resource_;
	std::shared_ptr<viscom::GPUProgram> shader_resource_;
	glm::mat4 model_matrix_;
public:
	SimpleGameMesh(std::shared_ptr<viscom::Mesh> mesh, std::shared_ptr<viscom::GPUProgram> shader) :
		MeshBase(mesh.get(), shader.get()),
		mesh_resource_(mesh), shader_resource_(shader)
	{}
	void transform(glm::mat4& t) {
		model_matrix_ *= t;
	}
	virtual void render(glm::mat4& vp, GLint isDebugMode = 0) const {
		MeshBase::render(vp, isDebugMode, model_matrix_);
	}
	virtual void render(std::function<void(void)> outsideUniformSetter, glm::mat4& vp, GLint isDebugMode = 0) const {
		MeshBase::render(outsideUniformSetter, vp, isDebugMode, model_matrix_);
	}
};


/* Mesh supporting use of shadow mapping.
* Extends MeshBase.
* Holds uniform locations of lightspace matrix and shadow map.
* Uses custom uniform render function of MeshBase.
*/
class ShadowReceivingMesh : public SimpleGameMesh {
    GLint uloc_lightspace_matrix_;
    GLint uloc_shadow_map_;
public:
    ShadowReceivingMesh(std::shared_ptr<viscom::Mesh> mesh, std::shared_ptr<viscom::GPUProgram> shader);
    void render(glm::mat4& vp, glm::mat4& lightspace, GLuint shadowMap, GLint isDebugMode = 0) const;
};

/* Generic mesh for post processing effects.
 * Holds a time for animated effects.
 * Manages several uniforms needed in post processing shader.
*/
class PostProcessingMesh : public SimpleGameMesh {
	GLint uloc_lightspace_matrix_;
	GLint uloc_shadow_map_;
	GLint uloc_time_;
	GLfloat time_;
public:
	PostProcessingMesh(std::shared_ptr<viscom::Mesh> mesh, std::shared_ptr<viscom::GPUProgram> shader);
	void setTime(double time) {
		time_ = (GLfloat) time;
	}
	void render(glm::mat4& vp, glm::mat4& lightspace, GLuint shadowMap, GLint isDebugMode = 0) const;
};

/* Simple synchronized mesh class extending MeshBase.
 * Owns mesh and shader resources.
 * Adds a model matrix for dynamic transformation.
 * Synchronizes the model matrix with SGCT nodes.
*/
class SynchronizedGameMesh : public MeshBase<viscom::SimpleMeshVertex> {
protected:
	std::shared_ptr<viscom::Mesh> mesh_resource_;
	std::shared_ptr<viscom::GPUProgram> shader_resource_;
	sgct::SharedObject<glm::mat4> sharedModelMatrix_;
public:
    glm::mat4 model_matrix_;
    SynchronizedGameMesh(std::shared_ptr<viscom::Mesh> mesh, std::shared_ptr<viscom::GPUProgram> shader) :
		MeshBase(mesh.get(), shader.get()),
		mesh_resource_(mesh), shader_resource_(shader)
	{}
	void transform(glm::mat4& t) {
		model_matrix_ *= t;
	}
	void preSync() { // master
		sharedModelMatrix_.setVal(model_matrix_);
	}
	void encode() { // master
		sgct::SharedData::instance()->writeObj(&sharedModelMatrix_);
	}
	void decode() { // slave
		sgct::SharedData::instance()->readObj(&sharedModelMatrix_);
	}
	void updateSyncedSlave() {
		model_matrix_ = sharedModelMatrix_.getVal();
	}
	void updateSyncedMaster() {
		//Can maybe stay empty
	}
	virtual void render(glm::mat4& vp, GLint isDebugMode = 0) const {
		MeshBase::render(vp, isDebugMode, model_matrix_);
	}
	virtual void render(std::function<void(void)> outsideUniformSetter, glm::mat4& vp, GLint isDebugMode = 0) const {
		MeshBase::render(outsideUniformSetter, vp, isDebugMode, model_matrix_);
	}
};

/* Mesh class for synchronized mesh instances on a grid.
 * Has no owned resources because it is managed by a mesh pool.
 * Synchronizes the instance buffer.
 * The instance buffer is dynamic.
*/
template <class PER_INSTANCE_DATA>
class SynchronizedInstancedMesh : public MeshBase<viscom::SimpleMeshVertex> {
private:
    sgct::SharedVector<PER_INSTANCE_DATA> shared_instance_buffer_;
    sgct::SharedObject<roomgame::InstanceBuffer> shared_gpu_instance_buffer_;
protected:
    roomgame::InstanceBuffer gpu_instance_buffer_;
    std::vector<PER_INSTANCE_DATA> instance_buffer_;
public:
    SynchronizedInstancedMesh(viscom::Mesh* mesh, viscom::GPUProgram* program, size_t pool_allocation_bytes)
        : MeshBase(mesh, program), gpu_instance_buffer_(pool_allocation_bytes)
    {
    
    }
    ~SynchronizedInstancedMesh() {
        glDeleteBuffers(1, &gpu_instance_buffer_.id_);
    }
    void preSync() { // master
        shared_instance_buffer_.setVal(instance_buffer_);
        shared_gpu_instance_buffer_.setVal(gpu_instance_buffer_);
    }
    void encode() { // master
        sgct::SharedData::instance()->writeVector(&shared_instance_buffer_);
        sgct::SharedData::instance()->writeObj(&shared_gpu_instance_buffer_);
    }
    void decode() { // slave
        sgct::SharedData::instance()->readVector(&shared_instance_buffer_);
        sgct::SharedData::instance()->readObj(&shared_gpu_instance_buffer_);
    }
    void updateSyncedSlave() {
        instance_buffer_ = shared_instance_buffer_.getVal();
        gpu_instance_buffer_ = shared_gpu_instance_buffer_.getVal();
        uploadInstanceBufferToGPU();
    }
    void updateSyncedMaster() {
        //Can maybe stay empty
        uploadInstanceBufferToGPU();
    }
private:
    void uploadInstanceBufferToGPU() {
        // instance buffer reached current capacity?
        if (instance_buffer_.size() * sizeof(PER_INSTANCE_DATA) > gpu_instance_buffer_.pool_allocation_bytes_ * gpu_instance_buffer_.num_reallocations_) {
            reallocGPUMemory();
        }
        glBindBuffer(GL_ARRAY_BUFFER, gpu_instance_buffer_.id_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, instance_buffer_.size() * sizeof(PER_INSTANCE_DATA), instance_buffer_.data());
    }
    void reallocGPUMemory() {
        // allocate new buffer and and copy data
        glBindBuffer(GL_COPY_READ_BUFFER, gpu_instance_buffer_.id_);
        GLuint tmpBuffer;
        glGenBuffers(1, &tmpBuffer);
        glBindBuffer(GL_COPY_WRITE_BUFFER, tmpBuffer);
        gpu_instance_buffer_.num_reallocations_++;
        glBufferData(GL_COPY_WRITE_BUFFER, gpu_instance_buffer_.pool_allocation_bytes_ * gpu_instance_buffer_.num_reallocations_, 0, GL_STATIC_COPY);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, gpu_instance_buffer_.num_instances_ * sizeof(PER_INSTANCE_DATA));
        // THIS IS WHAT MAKES REALLOCATION APPROACH ***UGLY***:
        glDeleteBuffers(1, &gpu_instance_buffer_.id_); // Delete old instance buffer
        glDeleteVertexArrays(1, &vao_); // Delete old VAO
        resetShader(); // Create new VAO and connect old vertex buffer
                       // Connect new instance buffer
        gpu_instance_buffer_.id_ = tmpBuffer;
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, gpu_instance_buffer_.id_);
        PER_INSTANCE_DATA::setAttribPointer();
    }
};

#endif