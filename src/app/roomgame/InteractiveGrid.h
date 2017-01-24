#ifndef INTERACTIVE_GRID_H_
#define INTERACTIVE_GRID_H_

#include <list>
#include <memory>
#include <sgct/Engine.h>
#include "core/gfx/GPUProgram.h"
#include "core/ApplicationNode.h"
#include "GridInteraction.h"

//template < int ARRAY_LEN >

class InteractiveGrid {
	// Data members
	float height_units_;
	float cell_size_;
	float z_distance_;
	std::vector<std::vector<GridCell>> cells_; //TODO better use array with templated size
	const size_t ROOM_MIN_SIZE_ = 2;
	// Render-related members
	GLuint vao_, vbo_;
	std::shared_ptr<viscom::GPUProgram> shader_;
	GLint mvp_uniform_location_, z_distance_uniform_location_;
	glm::mat4 model_matrix_;
	GLsizei num_vertices_;
	glm::mat4 last_sgctMVP_;
	// Input-related members
	glm::dvec2 last_mouse_position_; //TODO maybe replace with TUIO touch position
	std::list<GridInteraction*> interactions_;
	GridCell* last_room_start_cell_;
	GridCell* last_room_end_cell_;
public:
	InteractiveGrid(int columns, int rows, float height);
	~InteractiveGrid();
	// Helper functions
	void forEachCell(std::function<void(GridCell*)> callback);
	void forEachCell(std::function<void(GridCell*,bool*)> callback);
	void forEachCellInRange(GridCell* leftLower, GridCell* rightUpper, std::function<void(GridCell*)> callback);
	GridCell* getCellAt(glm::vec2 positionNDC);
	bool isInsideGrid(glm::vec2 positionNDC);
	bool isInsideCell(glm::vec2 positionNDC, GridCell* cell);
	glm::vec2 getNDC(glm::vec2 position);
	// Render functions
	void uploadVertexData();
	void loadShader(viscom::ApplicationNode* appNode);
	void render(glm::mat4 sgctMVP);
	void cleanup();
	// Input functions
	void onTouch(int touchID);
	void onRelease(int touchID);
	void onMouseMove(int touchID, double newx, double newy);

	void addRoom(GridCell* startCell, GridCell* endCell, bool isFinished);
};

#endif