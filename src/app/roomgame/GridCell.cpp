#include "GridCell.h"

GridCell::GridCell(float x, float y, size_t col_idx, size_t row_idx) {
	vertex_.x_position = x;
	vertex_.y_position = y;
	vertex_.build_state = EMPTY;
	vertex_.health_points = MAX_HEALTH;
	vertex_buffer_offset_ = 0;
	northNeighbor = 0, eastNeighbor = 0, southNeighbor = 0, westNeighbor = 0;
	col_idx_ = col_idx;
	row_idx_ = row_idx;
}

void GridCell::removeBuildState(GLuint vbo, unsigned int s, bool makeEmpty = false) {
    std::cout << "Old Build State: " << vertex_.build_state << "make empty is "<< makeEmpty << std::endl;
    if (makeEmpty) {
        vertex_.build_state = EMPTY;
    }
    else {
        vertex_.build_state &= ~s;
    }
    std::cout << "New Build State: " << vertex_.build_state << " Remove Stated " << s << std::endl;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER,
        vertex_buffer_offset_ + 2 * sizeof(GLfloat),
        sizeof(vertex_.build_state),
        (GLvoid*)&vertex_.build_state);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GridCell::addBuildState(GLuint vbo, unsigned int s) {
    vertex_.build_state |= s;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER,
        vertex_buffer_offset_ + 2 * sizeof(GLfloat),
        sizeof(vertex_.build_state),
        (GLvoid*)&vertex_.build_state);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GridCell::andBuildStateWith(GLuint vbo, unsigned int s) {
    vertex_.build_state &= s;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER,
        vertex_buffer_offset_ + 2 * sizeof(GLfloat),
        sizeof(vertex_.build_state),
        (GLvoid*)&vertex_.build_state);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GridCell::updateHealthPoints(GLuint vbo, unsigned int hp) {
	if (hp < MIN_HEALTH) hp = MIN_HEALTH;
	else if (hp > MAX_HEALTH) hp = MAX_HEALTH;
	vertex_.health_points = hp;
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER,
		vertex_buffer_offset_ + 2 * sizeof(GLfloat) + sizeof(GLint),
		sizeof(vertex_.health_points),
		(GLvoid*)&vertex_.health_points);
	if (vertex_.build_state != EMPTY) {
        // If the cell is not empty it should have a reference to its mesh instance in an instance buffer
        // Otherwise there went something WRONG when a MeshInstanceGrid called buildAt on this cell!
		RoomSegmentMesh::Instance::updateHealth(
			mesh_instance_.buffer_->id_,
			mesh_instance_.offset_instances_,
			vertex_.health_points);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


void GridCell::setMeshInstance(RoomSegmentMesh::InstanceBufferRange mesh_instance) {
	mesh_instance_ = mesh_instance;
}

void GridCell::setVertexBufferOffset(GLintptr o) {
	vertex_buffer_offset_ = o;
}

void GridCell::setVertexAttribPointer() {
	Vertex::setAttribPointer();
}

void GridCell::setNorthNeighbor(GridCell* N) {
	northNeighbor = N;
}

void GridCell::setEastNeighbor(GridCell* E) {
	eastNeighbor = E;
}

void GridCell::setSouthNeighbor(GridCell* S) {
	southNeighbor = S;
}

void GridCell::setWestNeighbor(GridCell* W) {
	westNeighbor = W;
}

GridCell* GridCell::getNorthNeighbor() {
	return northNeighbor;
}

GridCell* GridCell::getEastNeighbor() {
	return eastNeighbor;
}

GridCell* GridCell::getSouthNeighbor() {
	return southNeighbor;
}

GridCell* GridCell::getWestNeighbor() {
	return westNeighbor;
}

glm::vec2 GridCell::getPosition() {
	return glm::vec2(vertex_.x_position, vertex_.y_position);
}

float GridCell::getXPosition() {
	return vertex_.x_position;
}

float GridCell::getYPosition() {
	return vertex_.y_position;
}

unsigned int GridCell::getBuildState() {
	return vertex_.build_state;
}

unsigned int GridCell::getHealthPoints() {
	return vertex_.health_points;
}

size_t GridCell::getCol() {
	return col_idx_;
}

size_t GridCell::getRow() {
	return row_idx_;
}

GLintptr GridCell::getVertexBufferOffset() {
	return vertex_buffer_offset_;
}

size_t GridCell::getVertexBytes() {
	return sizeof(Vertex);
}

void* GridCell::getVertexPointer() {
	return &vertex_;
}

bool GridCell::isNorthOf(GridCell* other) {
	return this->row_idx_ > other->row_idx_;
}

bool GridCell::isEastOf(GridCell* other) {
	return this->col_idx_ > other->col_idx_;
}

bool GridCell::isSouthOf(GridCell* other) {
	return this->row_idx_ < other->row_idx_;
}

bool GridCell::isWestOf(GridCell* other) {
	return this->col_idx_ < other->col_idx_;
}

size_t GridCell::getColDistanceTo(GridCell* other) {
	if (other->col_idx_ > this->col_idx_)
		return other->col_idx_ - this->col_idx_;
	else
		return this->col_idx_ - other->col_idx_;
}

size_t GridCell::getRowDistanceTo(GridCell* other) {
	if (other->row_idx_ > this->row_idx_)
		return other->row_idx_ - this->row_idx_;
	else
		return this->row_idx_ - other->row_idx_;
}

float GridCell::getDistanceTo(GridCell* other) {
	return glm::distance(glm::vec2(col_idx_, row_idx_), glm::vec2(other->getCol(), other->getRow()));
}

RoomSegmentMesh::InstanceBufferRange GridCell::getMeshInstance() {
	if (vertex_.build_state  != EMPTY)
		return mesh_instance_;
	else
		return RoomSegmentMesh::InstanceBufferRange();
}