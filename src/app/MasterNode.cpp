/**
 * @file   MasterNode.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Implementation of the master application node.
 */

#include "MasterNode.h"
#include <imgui.h>
#include "core/imgui/imgui_impl_glfw_gl3.h"

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
		grid_(GRID_COLS_, GRID_ROWS_, GRID_HEIGHT_, &meshpool_),
		cellular_automaton_(&grid_, automaton_transition_time),
		interaction_mode_(GRID_PLACE_OUTER_INFLUENCE)
    {
    }

    MasterNode::~MasterNode() = default;


    void MasterNode::InitOpenGL() {
        ApplicationNodeImplementation::InitOpenGL();
		
		grid_.loadShader(GetApplication()->GetGPUProgramManager()); // for viewing build states...
		grid_.uploadVertexData(); // ...for debug purposes

		outerInfluence_->grid = &grid_;
    }

	/* Sync step 1: Master sets values of shared objects to the values of corresponding non-shared objects */
    void MasterNode::PreSync() {
        ApplicationNodeImplementation::PreSync();
		outerInfluence_->meshComponent->preSync();
        meshpool_.preSync();
		synchronized_grid_translation_.setVal(grid_.getTranslation());
		synchronized_automaton_transition_time_delta_.setVal(cellular_automaton_.getTimeDeltaNormalized());
		//TODO grid state sync only when automaton changed it
		synchronized_grid_state_.setVal(std::vector<roomgame::GRID_STATE_ELEMENT>(cellular_automaton_.getGridBuffer(), 
			cellular_automaton_.getGridBuffer() + cellular_automaton_.getGridBufferSize()));
    }

	/* Sync step 2: Master sends shared objects to the central SharedData singleton
	 * (Order in decoding on slaves must be the same as in encoding)
	*/
	void MasterNode::EncodeData() {
		ApplicationNodeImplementation::EncodeData();
		outerInfluence_->meshComponent->encode();
        meshpool_.encode();
		sgct::SharedData::instance()->writeObj<glm::vec3>(&synchronized_grid_translation_);
		sgct::SharedData::instance()->writeFloat(&synchronized_automaton_transition_time_delta_);
		sgct::SharedData::instance()->writeVector(&synchronized_grid_state_);
	}

	/* Sync step 3: Master updates its copies of cluster-wide variables with data it just synced
	 * (Slaves update their copies with the data they received)
	 * These copies are good because in contrast to SGCT shared objects they are not mutex locked on each access
	 * Note, that master does not need this update because its data is always up to date...
	 * ... but it is currently done to access the same variables with the same code on master and slaves
	*/
	void MasterNode::UpdateSyncedInfo() {
		ApplicationNodeImplementation::UpdateSyncedInfo();
		outerInfluence_->meshComponent->updateSyncedMaster();
        meshpool_.updateSyncedMaster();
		// Of course the following variables are redundant on master 
		// but help to write "unified" code in ApplicationNodeImplementation
		grid_translation_ = synchronized_grid_translation_.getVal();
		automaton_transition_time_delta_ = synchronized_automaton_transition_time_delta_.getVal();
		grid_state_ = synchronized_grid_state_.getVal();
		// GPU data:
		glBindTexture(GL_TEXTURE_2D, last_grid_state_texture_.id); // upload old grid state
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)GRID_COLS_, (GLsizei)GRID_ROWS_,
			last_grid_state_texture_.format, last_grid_state_texture_.datatype, grid_state_.data());
		grid_state_ = synchronized_grid_state_.getVal(); // fetch new grid state
		glBindTexture(GL_TEXTURE_2D, current_grid_state_texture_.id); // upload new grid state
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)GRID_COLS_, (GLsizei)GRID_ROWS_,
			current_grid_state_texture_.format, current_grid_state_texture_.datatype, grid_state_.data());
	}

	/* This SGCT stage is called only once before each frame, regardless of the number of viewports */
	void MasterNode::UpdateFrame(double t1, double t2) {
		ApplicationNodeImplementation::UpdateFrame(t1, t2);

		cellular_automaton_.setTransitionTime(automaton_transition_time);
		cellular_automaton_.setMoveDir(automaton_movedir_[0], automaton_movedir_[1]);
		cellular_automaton_.setBirthThreshold(automaton_birth_thd);
		cellular_automaton_.setDeathThreshold(automaton_death_thd);
		cellular_automaton_.setCollisionThreshold(automaton_collision_thd);
		cellular_automaton_.setOuterInfluenceNeighborThreshold(automaton_outer_infl_nbors_thd);
		cellular_automaton_.setDamagePerCell(automaton_damage_per_cell);
		cellular_automaton_.transition(clock_.t_in_sec);


        updateManager_.ManageUpdates(min(clock_.deltat(), 0.25));
	}

	/* This SGCT stage draws the scene to the current back buffer (left, right or both).
	This stage can be called several times per frame if multiple viewports and/or if stereoscopic rendering is active.*/
    void MasterNode::DrawFrame(FrameBuffer& fbo) {
        ApplicationNodeImplementation::DrawFrame(fbo);

		glm::mat4 viewProj = GetCamera()->GetViewPerspectiveMatrix();
        outerInfluence_->viewPersMat = viewProj;

		grid_.updateProjection(viewProj);
		fbo.DrawToFBO([&] { 
			if (render_mode_ == RenderMode::DBG) grid_.onFrame();
		});
		
    }

    void MasterNode::Draw2D(FrameBuffer& fbo) {
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

	void MasterNode::PostDraw() {
		ApplicationNodeImplementation::PostDraw();
	}

    void MasterNode::CleanUp() {
		grid_.cleanup();
		cellular_automaton_.cleanup();
        ApplicationNodeImplementation::CleanUp();
    }

	/* Switch input modes by keyboard on master
	 * [C] key down: camera control mode
	 * [V] key hit: tilt camera 45 degrees
	 * [S] key hit: start automaton and switch between outer influence and room placement
	 * [D] key down: debug render mode
	*/
    bool MasterNode::KeyboardCallback(int key, int scancode, int action, int mods) {
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
			GetCamera()->SetOrientation(GetCamera()->GetOrientation()*glm::quat(glm::vec3(glm::radians<float>(-5),0,0)));
		}
		else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
			if (interaction_mode_ == GRID_PLACE_OUTER_INFLUENCE) {
				interaction_mode_ = InteractionMode::GRID;
				cellular_automaton_.init(GetApplication()->GetGPUProgramManager());
			}
			else {
				interaction_mode_ = GRID_PLACE_OUTER_INFLUENCE;
			}
		}
		else if (key == GLFW_KEY_D) {
			if (action == GLFW_PRESS) {
				render_mode_ = RenderMode::DBG;
			}
			else if (action == GLFW_RELEASE) {
				render_mode_ = RenderMode::NORMAL;
			}
		}
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_KeyCallback(key, scancode, action, mods);
#endif
        return ApplicationNodeImplementation::KeyboardCallback(key, scancode, action, mods);
    }

	/* Mouse/touch controls camera and room-/outer influence placement
	 * When in "place outer influence"-mode, click to place outer influence
	 * When in camera mode, click and drag to move camera
	 * When in grid mode, click and drag to build room
	*/
    bool MasterNode::MouseButtonCallback(int button, int action) {
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (interaction_mode_ == InteractionMode::GRID) {
				if (action == GLFW_PRESS) grid_.onTouch(-1);
				else if (action == GLFW_RELEASE) grid_.onRelease(-1);
			}
			else if (interaction_mode_ == InteractionMode::GRID_PLACE_OUTER_INFLUENCE) {
				if (action == GLFW_PRESS) grid_.populateCircleAtLastMousePosition(5);
			}
			else if (interaction_mode_ == InteractionMode::CAMERA) {
//				if (action == GLFW_PRESS) camera_.onTouch();
//				else if (action == GLFW_RELEASE) camera_.onRelease();
			}
		}
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MouseButtonCallback(button, action, 0);
#endif
        return ApplicationNodeImplementation::MouseButtonCallback(button, action);
    }

	/* MousePosCallback constantly updates interaction targets with cursor position
	 * Workaround for missing cursor position in MouseButtonCallback:
	 * Interaction targets can use their last cursor position
	*/
    bool MasterNode::MousePosCallback(double x, double y) {
        //x *= 2;
		//glm::vec2 pos = FindIntersectionWithPlane(GetCamera()->GetPickRay({ x,y }));
        //x = pos.x;
        //y = pos.y;
		viscom::math::Line3<float> ray = GetCamera()->GetPickRay({ x,y });
		grid_.onMouseMove(-1, ray[0], ray[1]);
		grid_.onMouseMove(-1, x, y);
		//camera_.onMouseMove((float)x, (float)y);
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MousePositionCallback(x, y);
#endif
        return ApplicationNodeImplementation::MousePosCallback(x, y);
    }

	/* Mouse scroll events are used to zoom, when in camera mode */
    bool MasterNode::MouseScrollCallback(double xoffset, double yoffset) {
		if (interaction_mode_ == InteractionMode::CAMERA) {
//			camera_.onScroll((float)yoffset);
//			camera_->HandleMouse(0,0,)
			GetCamera()->SetPosition(GetCamera()->GetPosition() + glm::vec3(0, 0, (float)yoffset*0.1f));
		}
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_ScrollCallback(xoffset, yoffset);
#endif
        return ApplicationNodeImplementation::MouseScrollCallback(xoffset, yoffset);
    }

	bool MasterNode::CharCallback(unsigned int character, int mods) {
#ifndef VISCOM_CLIENTGUI
		ImGui_ImplGlfwGL3_CharCallback(character);
#endif
		return ApplicationNodeImplementation::CharCallback(character, mods);
	}
    
}