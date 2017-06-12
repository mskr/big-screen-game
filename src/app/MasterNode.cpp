/**
 * @file   MasterNode.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Implementation of the master application node.
 */

#include "MasterNode.h"
#include <imgui.h>

static float automaton_transition_time = 0.04f;
static int automaton_movedir_[2] = { 1,0 };
static float automaton_birth_thd = 0.4f;
static float automaton_death_thd = 0.5f;
static float automaton_collision_thd = 0.2f;
static int automaton_outer_infl_nbors_thd = 2;
static int automaton_damage_per_cell = 5;

namespace viscom {

    MasterNode::MasterNode(ApplicationNodeInternal* appNode) :
        ApplicationNodeImplementation{ appNode },
		grid_(GRID_COLS_, GRID_ROWS_, GRID_HEIGHT_NDC_, &meshpool_),
		cellular_automaton_(&grid_, automaton_transition_time),
		interaction_mode_(GRID_PLACE_OUTER_INFLUENCE),
		camera_(glm::mat4(1))
    {
    }

    MasterNode::~MasterNode() = default;


    void MasterNode::InitOpenGL()
    {
        ApplicationNodeImplementation::InitOpenGL();
		grid_.loadShader(appNode_->GetGPUProgramManager()); // for viewing build states...
		grid_.uploadVertexData(); // ...for debug purposes

		/* TODO
		The following code sets data for the meshpool shader (renderMeshInstance.vert/frag).
		This must be done on all nodes but the data has to be synced first.
		Concerning the textures of the automaton that are used for interpolation,
		syncing would require to download them from the master node GPU, then sync
		and then upload them on slave GPUs.
		Concerning transition time delta and grid parameters syncing is not necessary,
		because this is currently constant data.
		*/
		meshpool_.updateUniformEveryFrame("automatonTimeDelta", [&](GLint uloc) {
			glUniform1f(uloc, cellular_automaton_.getTimeDeltaNormalized());
		});
		meshpool_.updateUniformEveryFrame("gridTex", [&](GLint uloc) {
			if (!cellular_automaton_.isInitialized()) return;
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, cellular_automaton_.getLatestTexture());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			float borderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
			glUniform1i(uloc, 0);
		});
		meshpool_.updateUniformEveryFrame("gridTex_PrevState", [&](GLint uloc) {
			if (!cellular_automaton_.isInitialized()) return;
			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, cellular_automaton_.getPreviousTexture());
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			float borderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
			glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
			glUniform1i(uloc, 1);
		});
		meshpool_.updateUniformEveryFrame("gridDimensions", [&](GLint uloc) {
			glUniform2f(uloc, grid_.getNumColumns()*grid_.getCellSize(), grid_.getNumRows()*grid_.getCellSize());
		});
		meshpool_.updateUniformEveryFrame("gridTranslation", [&](GLint uloc) {
			glUniform3f(uloc, grid_.getTranslation().x, grid_.getTranslation().y, grid_.getTranslation().z);
		});
		meshpool_.updateUniformEveryFrame("gridCellSize", [&](GLint uloc) {
			glUniform1f(uloc, grid_.getCellSize());
		});
    }

	/* Sync step 1: Master sets values of shared objects to corresponding non-shared objects */
    void MasterNode::PreSync()
    {
        ApplicationNodeImplementation::PreSync();
		shared_camera_matrix_.setVal(camera_.getViewProjection());
    }

	/* Sync step 2: Master sends shared objects to the central SharedData singleton
	 * (Order in decoding on slaves must be the same as in encoding)
	*/
	void MasterNode::EncodeData() {
		ApplicationNodeImplementation::EncodeData();
		sgct::SharedData::instance()->writeObj(&shared_camera_matrix_);
	}

	/* Sync step 3: Master updates its copies of cluster-wide variables with data it just synced
	 * (Slaves update their copies with the data they received)
	*/
	void MasterNode::UpdateSyncedInfo() {
		ApplicationNodeImplementation::UpdateSyncedInfo();
		camera_matrix_ = camera_.getViewProjection();
	}

    void MasterNode::DrawFrame(FrameBuffer& fbo)
    {
        ApplicationNodeImplementation::DrawFrame(fbo);

		cellular_automaton_.setTransitionTime(automaton_transition_time);
		cellular_automaton_.setMoveDir(automaton_movedir_[0], automaton_movedir_[1]);
		cellular_automaton_.setBirthThreshold(automaton_birth_thd);
		cellular_automaton_.setDeathThreshold(automaton_death_thd);
		cellular_automaton_.setCollisionThreshold(automaton_collision_thd);
		cellular_automaton_.setOuterInfluenceNeighborThreshold(automaton_outer_infl_nbors_thd);
		cellular_automaton_.setDamagePerCell(automaton_damage_per_cell);
		cellular_automaton_.transition(clock_.t_in_sec);

		glm::mat4 proj = GetEngine()->getCurrentModelViewProjectionMatrix() * camera_.getViewProjection();

		grid_.updateProjection(proj);
		if (render_mode_ == RenderMode::DBUG) grid_.onFrame();
    }

    void MasterNode::Draw2D(FrameBuffer& fbo)
    {
        fbo.DrawToFBO([&]() {
#ifndef VISCOM_CLIENTGUI
            ImGui::ShowTestWindow();
#endif
            ImGui::SetNextWindowPos(ImVec2(700, 60), ImGuiSetCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiSetCond_FirstUseEver);
			ImGui::GetIO().FontGlobalScale = 1.5f;
			if (ImGui::Begin("Roomgame Controls")) {
				//ImGui::SetWindowFontScale(2.0f);
				ImGui::Text("Interaction mode: %s", (interaction_mode_ == GRID) ? "GRID" : 
					((interaction_mode_ == GRID_PLACE_OUTER_INFLUENCE) ? "GRID_PLACE_OUTER_INFLUENCE" : "CAMERA"));
				ImGui::Text("AUTOMATON");
				ImGui::SliderFloat("transition time", &automaton_transition_time, 0.017f, 1.0f);
				ImGui::SliderInt2("move direction", automaton_movedir_, -1, 1);
				ImGui::SliderFloat("BIRTH_THRESHOLD", &automaton_birth_thd, 0.0f, 1.0f);
				ImGui::SliderFloat("DEATH_THRESHOLD", &automaton_death_thd, 0.0f, 1.0f);
				ImGui::SliderFloat("ROOM_NBORS_AHEAD_THRESHOLD", &automaton_collision_thd, 0.0f, 1.0f);
				ImGui::SliderInt("OUTER_INFL_NBORS_THRESHOLD", &automaton_outer_infl_nbors_thd, 1, 8);
				ImGui::SliderInt("DAMAGE_PER_CELL", &automaton_damage_per_cell, 1, 100);
			}
			ImGui::End();
        });

        ApplicationNodeImplementation::Draw2D(fbo);
    }

    void MasterNode::CleanUp()
    {
		grid_.cleanup();
		cellular_automaton_.cleanup();
        ApplicationNodeImplementation::CleanUp();
    }

	/* Keys switch input modes
	 * [C] key down: camera control mode
	 * [V] key hit: tilt camera 45 degrees
	 * [S] key hit: start automaton and switch between outer influence and room placement
	 * [D] key down: debug render mode
	*/
    void MasterNode::KeyboardCallback(int key, int scancode, int action, int mods)
    {
		// Keys switch input modes
		static InteractionMode mode_before_switch_to_camera = interaction_mode_;
		if (key == GLFW_KEY_C) {
			if (action == GLFW_PRESS) {
				mode_before_switch_to_camera = interaction_mode_;
				interaction_mode_ = InteractionMode::CAMERA;
			}
			else if (action == GLFW_RELEASE) {
				interaction_mode_ = (InteractionMode)mode_before_switch_to_camera;
			}
		}
		else if (key == GLFW_KEY_V && action == GLFW_PRESS) {
			camera_.setXRotation(-glm::quarter_pi<float>());
		}
		else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
			if (interaction_mode_ == GRID_PLACE_OUTER_INFLUENCE) {
				interaction_mode_ = InteractionMode::GRID;
				cellular_automaton_.init(appNode_->GetGPUProgramManager());
			}
			else {
				interaction_mode_ = GRID_PLACE_OUTER_INFLUENCE;
			}
		}
		else if (key == GLFW_KEY_D) {
			if (action == GLFW_PRESS) {
				render_mode_ = RenderMode::DBUG;
			}
			else if (action == GLFW_RELEASE) {
				render_mode_ = RenderMode::NORMAL;
			}
		}
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_KeyCallback(key, scancode, action, mods);
#endif
        ApplicationNodeImplementation::KeyboardCallback(key, scancode, action, mods);
    }

    void MasterNode::CharCallback(unsigned int character, int mods)
    {
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_CharCallback(character);
#endif
        ApplicationNodeImplementation::CharCallback(character, mods);
    }

	/* Mouse/touch controles camera and room-/outer influence placement
	 * When in "place outer influence"-mode, click to place outer influence
	 * When in camera mode, click and drag to move camera
	 * When in grid mode, click and drag to build room
	*/
    void MasterNode::MouseButtonCallback(int button, int action)
    {
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (action == GLFW_PRESS) { mouseTest = true; }
			if (interaction_mode_ == InteractionMode::GRID) {
				if (action == GLFW_PRESS) grid_.onTouch(-1);
				else if (action == GLFW_RELEASE) grid_.onRelease(-1);
			}
			else if (interaction_mode_ == InteractionMode::GRID_PLACE_OUTER_INFLUENCE) {
				if (action == GLFW_PRESS) grid_.populateCircleAtLastMousePosition(5);
			}
			else if (interaction_mode_ == InteractionMode::CAMERA) {
				if (action == GLFW_PRESS) camera_.onTouch();
				else if (action == GLFW_RELEASE) camera_.onRelease();
			}
		}
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MouseButtonCallback(button, action, 0);
#endif
        ApplicationNodeImplementation::MouseButtonCallback(button, action);
    }

	/* MousePosCallback constantly updates interaction targets with cursor position
	 * Workaround for missing cursor position in MouseButtonCallback:
	 * Interaction targets can use their last cursor position
	*/
    void MasterNode::MousePosCallback(double x, double y)
    {
		if (interaction_mode_ == InteractionMode::GRID)
			grid_.onMouseMove(-1, x, y);
		else
			grid_.onMouseMove(-2, x, y);
		camera_.onMouseMove((float)x, (float)y);
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MousePositionCallback(x, y);
#endif
        ApplicationNodeImplementation::MousePosCallback(x, y);
    }

	/* Mouse scroll events are used to zoom, when in camera mode */
    void MasterNode::MouseScrollCallback(double xoffset, double yoffset)
    {
		if (interaction_mode_ == InteractionMode::CAMERA) camera_.onScroll((float)yoffset);
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_ScrollCallback(xoffset, yoffset);
#endif
        ApplicationNodeImplementation::MouseScrollCallback(xoffset, yoffset);
    }

}
