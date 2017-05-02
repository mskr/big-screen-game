#include "GameMesh.h"

ShadowReceivingMesh::ShadowReceivingMesh(std::shared_ptr<viscom::Mesh> mesh, std::shared_ptr<viscom::GPUProgram> shader) :
	GameMesh(mesh, shader)
{
	uloc_lightspace_matrix_ = shader->getUniformLocation("lightSpaceMatrix");
	uloc_shadow_map_ = shader->getUniformLocation("shadowMap");
}

void ShadowReceivingMesh::render(glm::mat4& vp, glm::mat4& lightspace, GLuint shadowMap, GLint isDebugMode) const {
	glUseProgram(shader_resource_->getProgramId());
	glUniformMatrix4fv(uloc_lightspace_matrix_, 1, GL_FALSE, &lightspace[0][0]);
	// Bind shadow map to texture unit **2** because
	// the MeshRenderable super-class already uses 0 (diffuse texture) and 1 (bump map)
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowMap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glUniform1i(uloc_shadow_map_, 2);
	GameMesh::render(vp, isDebugMode);
}