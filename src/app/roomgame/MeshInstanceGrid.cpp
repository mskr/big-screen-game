#include "MeshInstanceGrid.h"

MeshInstanceGrid::MeshInstanceGrid(size_t columns, size_t rows, float height, RoomSegmentMeshPool* meshpool) :
	RoomInteractiveGrid(columns, rows, height), meshpool_(meshpool)
{

}

/* Called only on master (resulting instance buffer is synced) */
void MeshInstanceGrid::addInstanceAt(GridCell* c, GLuint buildStateBits) {
    RoomSegmentMesh::Instance instance;
    instance.scale = cell_size_ / 1.98f; // assume model extends [-1,1]^3
    instance.translation = translation_; // grid translation
    instance.translation += glm::vec3(c->getPosition(), 0.0f); // + relative cell translation
    instance.translation += glm::vec3(cell_size_ / 2.0f, -cell_size_ / 2.0f, 0.0f); // + origin to middle of cell
    instance.buildState = buildStateBits;
    instance.health = c->getHealthPoints();
    meshpool_->filter(buildStateBits, [&](GLuint renderableBuildState) {
        RoomSegmentMesh* mesh = meshpool_->getMeshOfType(renderableBuildState);
        RoomSegmentMesh::InstanceBufferRange bufrange = mesh->addInstanceUnordered(instance);
        c->pushMeshInstance(bufrange);
    });
}

/* Called only on master (resulting instance buffer is synced) */
void MeshInstanceGrid::removeInstanceAt(GridCell* c) {
    RoomSegmentMesh::InstanceBufferRange bufrange;
    while ((bufrange = c->popMeshInstance()).mesh_) {
        bufrange.mesh_->removeInstanceUnordered(bufrange.offset_instances_);
    }
}

void MeshInstanceGrid::buildAt(GridCell* c, std::function<void(GridCell*)> callback) {
    GLuint current = c->getBuildState();
    callback(c);
    GLuint newSt = c->getBuildState();
    c->setBuildState(current);
    if (current == newSt) return;
    else if (current == GridCell::EMPTY) addInstanceAt(c, newSt);
    else if (newSt == GridCell::EMPTY) removeInstanceAt(c);
    else {
        removeInstanceAt(c);
        addInstanceAt(c, newSt);
    }
    c->setBuildState(newSt);
    c->updateBuildState(vbo_);
}

void MeshInstanceGrid::buildAt(GridCell* c, GLuint newState, BuildMode buildMode) {
    GLuint current = c->getBuildState();
    GLuint moddedState;
    switch (buildMode) {
    case BuildMode::Additive:
        moddedState = current | newState;
        break;
    case BuildMode::Replace:
        moddedState = newState;
        break;
    case BuildMode::RemoveSpecific:
        moddedState = (current | newState)^newState;
        break;
    }
    if (current == moddedState) return;
    else if (current == GridCell::EMPTY) addInstanceAt(c, moddedState);
    else if (moddedState == GridCell::EMPTY) removeInstanceAt(c);
    else {
        removeInstanceAt(c);
        addInstanceAt(c, moddedState);
    }
    c->setBuildState(moddedState);
    c->updateBuildState(vbo_);
}

void MeshInstanceGrid::buildAt(size_t col, size_t row, std::function<void(GridCell*)> callback) {
    GridCell* maybeCell = getCellAt(col, row);
    if (maybeCell) buildAt(maybeCell, callback);
}

void MeshInstanceGrid::buildAt(size_t col, size_t row, GLuint newState, BuildMode buildMode) {
    GridCell* maybeCell = getCellAt(col, row);
    if (maybeCell) buildAt(maybeCell, newState, buildMode);
}