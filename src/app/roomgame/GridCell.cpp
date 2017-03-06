#include "GridCell.h"

GridCell::GridCell(float x, float y, size_t col_idx, size_t row_idx) {
	vertex_.x_position = x;
	vertex_.y_position = y;
	vertex_.build_state = BuildState::EMPTY;
	vertex_.health_points = 1.0f;
	vertex_buffer_offset_ = 0;
	northNeighbor = 0, eastNeighbor = 0, southNeighbor = 0, westNeighbor = 0;
	col_idx_ = col_idx;
	row_idx_ = row_idx;
}

void GridCell::setVertexBufferOffset(GLintptr o) {
	vertex_buffer_offset_ = o;
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

void GridCell::setVertexAttribPointer() {
	Vertex::setAttribPointer();
}

void GridCell::updateBuildState(GLuint vbo, BuildState s) {
	vertex_.build_state = s;
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER,
		vertex_buffer_offset_ + sizeof(vertex_.x_position) + sizeof(vertex_.y_position),
		sizeof(vertex_.build_state),
		(GLvoid*)&vertex_.build_state);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
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

int GridCell::getBuildState() {
	return (int)vertex_.build_state;
}

float GridCell::getHealthPoints() {
	return vertex_.health_points;
}

size_t GridCell::getCol() {
	return col_idx_;
}

size_t GridCell::getRow() {
	return row_idx_;
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

RoomSegmentMesh::InstanceBufferRange GridCell::getMeshInstance() {
	if (vertex_.build_state != BuildState::EMPTY)
		return mesh_instance_;
	else
		return RoomSegmentMesh::InstanceBufferRange();
}

void GridCell::setMeshInstance(RoomSegmentMesh::InstanceBufferRange mesh_instance) {
	mesh_instance_ = mesh_instance;
}