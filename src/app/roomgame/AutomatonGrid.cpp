#include "AutomatonGrid.h"
#include "GPUCellularAutomaton.h"

AutomatonGrid::AutomatonGrid(size_t cols, size_t rows, float height, RoomSegmentMeshPool* meshpool) :
	MeshInstanceGrid(cols, rows, height, meshpool)
{
	automaton_ = 0;
}

void AutomatonGrid::setCellularAutomaton(GPUCellularAutomaton* automaton) {
	automaton_ = automaton;
}

void AutomatonGrid::onMeshpoolInitialized() {
	meshpool_->updateUniformEveryFrame("automatonTimeDelta", [&](GLint uloc) {
		glUniform1f(uloc, automaton_->getTimeDeltaNormalized());
	});
	meshpool_->updateUniformEveryFrame("gridTex", [&](GLint uloc) {
		if (!automaton_->isInitialized()) return;
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, automaton_->getLatestTexture());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		float borderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
		glUniform1i(uloc, 0);
	});
	meshpool_->updateUniformEveryFrame("gridDimensions", [&](GLint uloc) {
		glUniform2f(uloc, getNumColumns()*cell_size_, getNumRows()*cell_size_);
	});
	meshpool_->updateUniformEveryFrame("gridTranslation", [&](GLint uloc) {
		glUniform3f(uloc, translation_.x, translation_.y, translation_.z);
	});
	meshpool_->updateUniformEveryFrame("gridCellSize", [&](GLint uloc) {
		glUniform1f(uloc, cell_size_);
	});
}

void AutomatonGrid::buildAt(size_t col, size_t row, GridCell::BuildState state) {
	// Called on user input
	MeshInstanceGrid::buildAt(col, row, state);
	GridCell* c = getCellAt(col, row);
	if (!c) return;
	c->updateHealthPoints(vbo_, GridCell::MAX_HEALTH);
	// Route results to automaton
	automaton_->updateCell(col, row, state, c->getHealthPoints());
}

void AutomatonGrid::updateGridOnly(size_t col, size_t row, GridCell::BuildState state, int hp) {
	// Called on automaton transitions
	MeshInstanceGrid::buildAt(col, row, state);
	GridCell* c = getCellAt(col, row);
	if (!c) return;
	c->updateHealthPoints(vbo_, hp);
}