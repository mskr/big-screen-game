#ifndef GAME_MESH_H
#define GAME_MESH_H

#include "core/gfx/mesh/Mesh.h"
#include "sgct\Engine.h"
#include <memory>
#include "core/gfx/GPUProgram.h"
#include "core/gfx/mesh/MeshRenderable.h"
#include "../Vertices.h"

template <class VERTEX_LAYOUT>
class GameMesh : public viscom::MeshRenderable {
protected:
	std::shared_ptr<viscom::Mesh> mesh_resource_;
	std::shared_ptr<viscom::GPUProgram> shader_resource_;
	glm::mat4 model_matrix_;
	GLint uloc_view_projection_;
	GLint uloc_debug_mode_flag_;
public:
	GameMesh(std::shared_ptr<viscom::Mesh> mesh, std::shared_ptr<viscom::GPUProgram> shader) :
		viscom::MeshRenderable(mesh.get(), VERTEX_LAYOUT::CreateVertexBuffer(mesh.get()), shader.get()),
		mesh_resource_(mesh), shader_resource_(shader) {
		NotifyRecompiledShader<VERTEX_LAYOUT>(shader.get());
		uloc_view_projection_ = shader->getUniformLocation("viewProjectionMatrix");
		uloc_debug_mode_flag_ = shader->getUniformLocation("isDebugMode");
	}
	void transform(glm::mat4& t) {
		model_matrix_ *= t;
	}
	virtual void render(glm::mat4& vp, GLint isDebugMode = 0) const {
		glUseProgram(shader_resource_->getProgramId());
		glUniformMatrix4fv(uloc_view_projection_, 1, GL_FALSE, &vp[0][0]);
		if (isDebugMode == 1) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glUniform1i(uloc_debug_mode_flag_, isDebugMode);
		Draw(model_matrix_);
	}
};

class ShadowReceivingMesh : public GameMesh<viscom::SimpleMeshVertex> {
	GLint uloc_lightspace_matrix_;
	GLint uloc_shadow_map_;
public:
	ShadowReceivingMesh(std::shared_ptr<viscom::Mesh> mesh, std::shared_ptr<viscom::GPUProgram> shader);
	void render(glm::mat4& vp, glm::mat4& lightspace, GLuint shadowMap, GLint isDebugMode = 0) const;
};

#endif