#ifndef GAME_MESH_H
#define GAME_MESH_H

#include <memory>
#include "sgct\Engine.h"
#include "core/gfx/mesh/Mesh.h"
#include "core/gfx/GPUProgram.h"
#include "core/gfx/mesh/MeshRenderable.h"
#include "../Vertices.h"

template <class VERTEX_LAYOUT>
class GameMesh : public viscom::MeshRenderable {
protected:
	std::shared_ptr<viscom::Mesh> mesh_resource;
	std::shared_ptr<viscom::GPUProgram> shader_resource;
	glm::mat4 model_matrix;
	GLint uloc_view_projection;
public:
	GameMesh(std::shared_ptr<viscom::Mesh> mesh, std::shared_ptr<viscom::GPUProgram> shader) :
		viscom::MeshRenderable(mesh.get(), VERTEX_LAYOUT::CreateVertexBuffer(mesh.get()), shader.get()),
		mesh_resource(mesh), shader_resource(shader) {
		NotifyRecompiledShader<VERTEX_LAYOUT>(shader.get());
		uloc_view_projection = shader->getUniformLocation("viewProjectionMatrix");
	}
	void transform(glm::mat4& t) {
		model_matrix *= t;
	}
	virtual void render(glm::mat4& vp) const {
		glUseProgram(shader_resource->getProgramId());
		glUniformMatrix4fv(uloc_view_projection, 1, GL_FALSE, &vp[0][0]);
		Draw(model_matrix);
	}
};

class ShadowReceivingMesh : public GameMesh<viscom::SimpleMeshVertex> {
	GLint uloc_lightspace_matrix;
	GLint uloc_shadow_map;
public:
	ShadowReceivingMesh(std::shared_ptr<viscom::Mesh> mesh, std::shared_ptr<viscom::GPUProgram> shader);
	void render(glm::mat4& vp, glm::mat4& lightspace, GLuint shadowMap) const {
		glUseProgram(shader_resource->getProgramId());
		glUniformMatrix4fv(uloc_view_projection, 1, GL_FALSE, &vp[0][0]);
		glUniformMatrix4fv(uloc_lightspace_matrix, 1, GL_FALSE, &lightspace[0][0]);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, shadowMap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		glUniform1i(uloc_shadow_map, 2);
		Draw(model_matrix);
	}
};

#endif