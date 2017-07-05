#ifndef INTERACTIVE_GRID_H_
#define INTERACTIVE_GRID_H_

#include <list>
#include <memory>
//#include <sgct/Engine.h>
#include "core/gfx/GPUProgram.h"
#include "core/ApplicationNodeBase.h"
#include "GridInteraction.h"

/* Renderable and clickable grid.
 * Container for grid cell objects.
 * Supports a simple debug visualization (see onFrame function).
 * Provides a number of helper functions.
*/
class InteractiveGrid {
protected:
	// Data members
	float height_units_;
	float cell_size_;
	std::vector<std::vector<GridCell>> cells_;
	// Render-related members
	GLuint vao_, vbo_;
	std::shared_ptr<viscom::GPUProgram> shader_;
	GLint mvp_uniform_location_;
	glm::vec3 translation_;
	GLsizei num_vertices_;
	glm::mat4 last_view_projection_;
	// Input-related members
	glm::dvec2 last_mouse_position_;
	std::list<GridInteraction*> interactions_;

public:

    /* Computes cell positions by iteratively adding (height/rows, height/rows) to (-1, -1) */
	InteractiveGrid(size_t columns, size_t rows, float height);
	~InteractiveGrid();


	// Helper functions
	GridCell* InteractiveGrid::getClosestWallCell(glm::vec2 pos);
	void forEachCell(std::function<void(GridCell*)> callback);
	void forEachCell(std::function<void(GridCell*,bool*)> callback);
	void forEachCellInRange(GridCell* leftLower, GridCell* rightUpper, std::function<void(GridCell*)> callback);
	void forEachCellInRange(GridCell* leftLower, GridCell* rightUpper, std::function<void(GridCell*,bool*)> callback);

    /* Binary search on cells */
	GridCell* getCellAt(glm::vec2 positionNDC);

	bool isInsideGrid(glm::vec2 positionNDC);
	bool isInsideCell(glm::vec2 positionNDC, GridCell* cell);
	bool isColumnEmptyBetween(size_t col, size_t startRow, size_t endRow);
	bool isRowEmptyBetween(size_t row, size_t startCol, size_t endCol);


	// Getters
	float getCellSize();
	size_t getNumColumns();
	size_t getNumRows();
	size_t getNumCells();
	GridCell* getCellAt(size_t col, size_t row);
	glm::vec3 getTranslation() { return translation_; }


	// Render functions
	void uploadVertexData();
	virtual void loadShader(viscom::GPUProgramManager mgr);
	void onFrame();
	void cleanup();
	void translate(float dx, float dy, float dz);


	// Input and interaction functions
	virtual void onTouch(int touchID);
	virtual void onRelease(int touchID);
	virtual void onMouseMove(int touchID, double newx, double newy);
<<<<<<< HEAD
	glm::vec2 getNDC(glm::vec2 position); // converts position of a cell in model-space into position as seen on the screen
=======

    /* Transform original cell position into what the user sees */
	glm::vec2 getNDC(glm::vec2 position);

    /* Remember camera projection for later being able to perform cell selection on user input */
>>>>>>> 9bf64b6700432ea59092fea70d3de39670bcde74
	void updateProjection(glm::mat4&);


	// Functions for grid modification
	virtual void buildAt(size_t col, size_t row, GridCell::BuildState buildState);
	void buildAtLastMousePosition(GridCell::BuildState buildState);
};

#endif