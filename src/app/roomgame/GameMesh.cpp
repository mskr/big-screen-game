#include "GameMesh.h"

ShadowReceivingMesh::ShadowReceivingMesh(std::shared_ptr<viscom::Mesh> mesh, std::shared_ptr<viscom::GPUProgram> shader) :
	GameMesh(mesh, shader)
{
	uloc_lightspace_matrix = shader->getUniformLocation("lightSpaceMatrix");
	uloc_shadow_map = shader->getUniformLocation("shadowMap");
}