#ifndef GRID_CELL_H
#define GRID_CELL_H

//#include <sgct/Engine.h>
#include "RoomSegmentMesh.h"

/* Represents one cell of the grid.
 * Defines the list of build states used by the roomgame.
 * Supports a debug rendering of grid cells as GL_POINTS.
 * Holds positions, build state and health points.
 * Holds neighbor cells.
 * Holds its own column and row in the containing grid.
 * Holds reference to an instance buffer range, where a mesh instance is stored that is renderd at this cell's position.
 * Privides a number of helper methods.
*/
class GridCell {
public:
    /*
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
        INSIDE_ROOM_INFECTED = 11,
		OUTER_INFLUENCE = 12
	};*/

    static const GLuint EMPTY = 0;
    static const GLuint INSIDE_ROOM = 1;
    static const GLuint CORNER = 2;
    static const GLuint WALL = 4;
    static const GLuint TOP = 8;
    static const GLuint BOTTOM = 16;
    static const GLuint RIGHT = 32;
    static const GLuint LEFT = 64;
    static const GLuint INVALID = 128;
    static const GLuint SOURCE = 256;
    static const GLuint INFECTED = 512;
    static const GLuint OUTER_INFLUENCE = 1024;

	static const int MAX_HEALTH = 100;
	static const int MIN_HEALTH = 0;
private:
	struct Vertex {
		GLfloat x_position;
		GLfloat y_position;
		GLuint build_state;
		GLint health_points;
		static const void setAttribPointer() {
			const GLint posAttrLoc = 0;
			const GLint buildStateAttrLoc = 1;
			const GLint healthAttrLoc = 2;
			glVertexAttribPointer(posAttrLoc, 2, GL_FLOAT, false,
				sizeof(Vertex),
				(GLvoid*)0);
			glVertexAttribIPointer(buildStateAttrLoc, 1, GL_INT,
				sizeof(Vertex),
				(GLvoid*)(2 * sizeof(float)));
			glVertexAttribIPointer(healthAttrLoc, 1, GL_INT,
				sizeof(Vertex),
				(GLvoid*)(2 * sizeof(float) + sizeof(GLuint)));
			glEnableVertexAttribArray(posAttrLoc);
			glEnableVertexAttribArray(buildStateAttrLoc);
			glEnableVertexAttribArray(healthAttrLoc);
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
	bool isSource_ = false;
public:
	GridCell(float x, float y, size_t col_idx, size_t row_idx);
	~GridCell() = default;
	void setIsSource(bool state);
	bool getIsSource();
    void removeBuildState(GLuint vbo, GLuint s, bool makeEmpty);
    void addBuildState(GLuint vbo, GLuint s);
    void updateHealthPoints(GLuint vbo, int hp);
	void setMeshInstance(RoomSegmentMesh::InstanceBufferRange mesh_instance);
	static void setVertexAttribPointer();
	void setVertexBufferOffset(GLintptr o);
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
	GLuint getBuildState();
	int getHealthPoints();
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
	float getDistanceTo(GridCell* other);
    RoomSegmentMesh::InstanceBufferRange getMeshInstance();
};

#endif