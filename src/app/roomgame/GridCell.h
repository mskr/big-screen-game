#ifndef GRID_CELL_H
#define GRID_CELL_H

#include <sgct/Engine.h>
#include "RoomSegmentMesh.h"

class GridCell {
public:
	enum BuildState {
		EMPTY = 0,
		INSIDE_ROOM = 1,
		LEFT_UPPER_CORNER = 2,
		RIGHT_UPPER_CORNER = 3,
		LEFT_LOWER_CORNER = 4,
		RIGHT_LOWER_CORNER = 5,
		WALL_LEFT = 6,
		WALL_RIGHT = 7,
		WALL_TOP = 8,
		WALL_BOTTOM = 9,
		INVALID = 10,
		OUTER_INFLUENCE = 11
	};
	static const int MAX_HEALTH = 100;
	static const int MIN_HEALTH = 0;
private:
	struct Vertex {
		GLfloat x_position;
		GLfloat y_position;
		GLint build_state;
		GLint health_points;
		static const void setAttribPointer() {
			glVertexAttribPointer(0, 2, GL_FLOAT, false,
				sizeof(Vertex),
				(GLvoid*)0);
			glVertexAttribIPointer(1, 1, GL_INT,
				sizeof(Vertex),
				(GLvoid*)(2 * sizeof(float)));
			glVertexAttribIPointer(2, 1, GL_INT,
				sizeof(Vertex),
				(GLvoid*)(2 * sizeof(float) + sizeof(BuildState)));
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
		}
	} vertex_;
	GLintptr vertex_buffer_offset_;
	GridCell* northNeighbor;
	GridCell* eastNeighbor;
	GridCell* southNeighbor;
	GridCell* westNeighbor;
	size_t col_idx_;
	size_t row_idx_;
	RoomSegmentMesh::InstanceBufferRange mesh_instance_;
public:
	GridCell(float x, float y, size_t col_idx, size_t row_idx);
	~GridCell() = default;
	static void setVertexAttribPointer();
	void updateBuildState(GLuint vbo, BuildState s);
	void setNorthNeighbor(GridCell* N);
	void setEastNeighbor(GridCell* E);
	void setSouthNeighbor(GridCell* S);
	void setWestNeighbor(GridCell* W);
	GridCell* getNorthNeighbor();
	GridCell* getEastNeighbor();
	GridCell* getSouthNeighbor();
	GridCell* getWestNeighbor();
	glm::vec2 getPosition();
	float getXPosition();
	float getYPosition();
	int getBuildState();
	int getHealthPoints();
	void setVertexBufferOffset(GLintptr o);
	GLintptr getVertexBufferOffset();
	static size_t getVertexBytes();
	void* getVertexPointer();
	size_t getCol();
	size_t getRow();
	bool isNorthOf(GridCell* other);
	bool isEastOf(GridCell* other);
	bool isSouthOf(GridCell* other);
	bool isWestOf(GridCell* other);
	size_t getColDistanceTo(GridCell* other);
	size_t getRowDistanceTo(GridCell* other);
	RoomSegmentMesh::InstanceBufferRange getMeshInstance();
	void setMeshInstance(RoomSegmentMesh::InstanceBufferRange mesh_instance);
};

#endif