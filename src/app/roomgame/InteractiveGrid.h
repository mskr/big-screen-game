#ifndef INTERACTIVE_GRID_H_
#define INTERACTIVE_GRID_H_

#include <list>
#include <memory>
#include <sgct/Engine.h>
#include "core/gfx/GPUProgram.h"
#include "core/ApplicationNode.h"
#include "Room.h"
#include "GridInteraction.h"

class RoomSegmentMeshPool;
#include "RoomSegmentMeshPool.h"

//template < int ARRAY_LEN >
class InteractiveGrid {
	// Data members
	float height_units_;
	float cell_size_;
	std::vector<std::vector<GridCell>> cells_; //TODO better use array with templated size
	std::vector<Room*> rooms_;
	// Render-related members
	GLuint vao_, vbo_;
	std::shared_ptr<viscom::GPUProgram> shader_;
	GLint mvp_uniform_location_;
	glm::mat4 model_matrix_;
	GLsizei num_vertices_;
	glm::mat4 last_sgctMVP_;
	RoomSegmentMeshPool* meshpool_;
	// Input-related members
	glm::dvec2 last_mouse_position_; //TODO maybe replace with TUIO touch position
	std::list<GridInteraction*> interactions_;
public:
	InteractiveGrid(int columns, int rows, float height);
	~InteractiveGrid();
	// Helper functions
	void forEachCell(std::function<void(GridCell*)> callback);
	void forEachCell(std::function<void(GridCell*,bool*)> callback);
	void forEachCellInRange(GridCell* leftLower, GridCell* rightUpper, std::function<void(GridCell*)> callback);
	void forEachCellInRange(GridCell* leftLower, GridCell* rightUpper, std::function<void(GridCell*,bool*)> callback);
	GridCell* getCellAt(glm::vec2 positionNDC);
	GridCell* getCellAt(size_t col, size_t row);
	bool isInsideGrid(glm::vec2 positionNDC);
	bool isInsideCell(glm::vec2 positionNDC, GridCell* cell);
	glm::vec2 getNDC(glm::vec2 position);
	bool isColumnEmptyBetween(size_t col, size_t startRow, size_t endRow);
	bool isRowEmptyBetween(size_t row, size_t startCol, size_t endCol);
	float getCellSize();
	size_t getNumColumns();
	size_t getNumRows();
	size_t getNumCells();
	// Render functions
	void uploadVertexData();
	void loadShader(viscom::ApplicationNode* appNode);
	void render(glm::mat4 sgctMVP);
	void cleanup();
	// Input functions
	void onTouch(int touchID);
	void onRelease(int touchID);
	void onMouseMove(int touchID, double newx, double newy);

	Room::CollisionType resizeRoomUntilCollision(Room* room, GridCell* startCell, GridCell* lastCell, GridCell* currentCell);
	void updateBuildStateAt(size_t col, size_t row, GridCell::BuildState buildState);
	void setRoomSegmentMeshPool(RoomSegmentMeshPool* meshpool);
};

#endif