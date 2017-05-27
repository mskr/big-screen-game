/**
 * @file   ApplicationNodeImplementation.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.30
 *
 * @brief  Implementation of the application node class.
 */

#include "ApplicationNodeImplementation.h"
#include "Vertices.h"
#include <imgui.h>
#include "core/gfx/mesh/MeshRenderable.h"
#include "core/imgui/imgui_impl_glfw_gl3.h"
#include <iostream>

#define GRID_COLUMNS 64
#define GRID_ROWS 64

static float automaton_transition_time = 0.04f;
static int automaton_movedir_[2] = { 1,0 };
static float automaton_birth_thd = 0.4f;
static float automaton_death_thd = 0.5f;
static float automaton_collision_thd = 0.2f;
static int automaton_outer_infl_nbors_thd = 2;
static int automaton_damage_per_cell = 5;

namespace viscom {

	ApplicationNodeImplementation::ApplicationNodeImplementation(ApplicationNode* appNode) :
		appNode_{ appNode },
		meshpool_(GRID_COLUMNS * GRID_ROWS),
		grid_(GRID_COLUMNS, GRID_ROWS, 2.0f, &meshpool_),
		interaction_mode_(GRID_PLACE_OUTER_INFLUENCE),
		camera_(glm::mat4(1)),
		cellular_automaton_(&grid_, automaton_transition_time),
		render_mode_(NORMAL),
		clock_{0.0}
    {
    }

    ApplicationNodeImplementation::~ApplicationNodeImplementation() = default;

    void ApplicationNodeImplementation::PreWindow()
    {
    }

    void ApplicationNodeImplementation::InitOpenGL()
    {
		meshpool_.loadShader(appNode_->GetGPUProgramManager());
		meshpool_.addMesh({ GridCell::BuildState::INSIDE_ROOM },
							appNode_->GetMeshManager().GetResource("/models/roomgame_models/floor.obj"));
		meshpool_.addMesh({ GridCell::BuildState::LEFT_LOWER_CORNER,
							GridCell::BuildState::LEFT_UPPER_CORNER,
							GridCell::BuildState::RIGHT_LOWER_CORNER,
							GridCell::BuildState::RIGHT_UPPER_CORNER,
							GridCell::BuildState::INVALID },
							appNode_->GetMeshManager().GetResource("/models/roomgame_models/corner.obj"));
		meshpool_.addMesh({ GridCell::BuildState::WALL_BOTTOM,
							GridCell::BuildState::WALL_TOP,
							GridCell::BuildState::WALL_RIGHT,
							GridCell::BuildState::WALL_LEFT },
							appNode_->GetMeshManager().GetResource("/models/roomgame_models/wall.obj"));
		meshpool_.addMesh({ GridCell::BuildState::OUTER_INFLUENCE },
							appNode_->GetMeshManager().GetResource("/models/roomgame_models/latticeplane.obj"));

		meshpool_.updateUniformEveryFrame("t_sec", [this](GLint uloc) {
			glUniform1f(uloc, (float)clock_.t_in_sec);
		});

		grid_.onMeshpoolInitialized();
		grid_.loadShader(appNode_->GetGPUProgramManager());
		grid_.uploadVertexData();

		ImGui::GetIO().FontGlobalScale = 1.5f;

		backgroundMesh_ = new ShadowReceivingMesh(
			appNode_->GetMeshManager().GetResource("/models/roomgame_models/textured_4vertexplane/textured_4vertexplane.obj"),
			appNode_->GetGPUProgramManager().GetResource("applyTextureAndShadow",
				std::initializer_list<std::string>{ "applyTextureAndShadow.vert", "applyTextureAndShadow.frag" }));
		backgroundMesh_->transform(glm::scale(glm::translate(glm::mat4(1), 
			glm::vec3(0,-grid_.getCellSize(),-0.001f/*TODO better remove the z bias and use thicker meshes*/)), glm::vec3(1.0f)));

		shadowMap_ = new ShadowMap(1024, 1024);
		shadowMap_->setLightMatrix(glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0), glm::vec3(0, 1, 0)));
		
		GetEngine()->setNearAndFarClippingPlanes(0.1f, 100.0f);
    }

    void ApplicationNodeImplementation::PreSync()
    {
    }

    void ApplicationNodeImplementation::UpdateSyncedInfo()
    {
    }

    void ApplicationNodeImplementation::UpdateFrame(double currentTime, double elapsedTime)
    {
		cellular_automaton_.setTransitionTime(automaton_transition_time);
		cellular_automaton_.setMoveDir(automaton_movedir_[0], automaton_movedir_[1]);
		cellular_automaton_.setBirthThreshold(automaton_birth_thd);
		cellular_automaton_.setDeathThreshold(automaton_death_thd);
		cellular_automaton_.setCollisionThreshold(automaton_collision_thd);
		cellular_automaton_.setOuterInfluenceNeighborThreshold(automaton_outer_infl_nbors_thd);
		cellular_automaton_.setDamagePerCell(automaton_damage_per_cell);
		cellular_automaton_.transition(currentTime);
		clock_.t_in_sec = currentTime;
    }

    void ApplicationNodeImplementation::ClearBuffer(FrameBuffer& fbo)
    {
        fbo.DrawToFBO([]() {
            auto colorPtr = sgct::Engine::instance()->getClearColor();
            glClearColor(colorPtr[0], colorPtr[1], colorPtr[2], colorPtr[3]);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        });

		shadowMap_->DrawToFBO([&]() {
			glClearDepth(1.0f);
			glClear(GL_DEPTH_BUFFER_BIT);
		});
    }

    void ApplicationNodeImplementation::DrawFrame(FrameBuffer& fbo)
    {
		glm::mat4 proj = GetEngine()->getCurrentModelViewProjectionMatrix() * camera_.getViewProjection();

        //TODO Is the engine matrix really needed here?
		glm::mat4 lightspace = GetEngine()->getCurrentModelViewProjectionMatrix() * shadowMap_->getLightMatrix();

		grid_.updateProjection(proj);
		
		shadowMap_->DrawToFBO([&]() {
			meshpool_.renderAllMeshesExcept(lightspace, GridCell::BuildState::OUTER_INFLUENCE, 1);
		});
		
        fbo.DrawToFBO([&]() {
			backgroundMesh_->render(proj, lightspace, shadowMap_->get(), (render_mode_ == RenderMode::DBUG) ? 1 : 0);
			glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			meshpool_.renderAllMeshes(proj, 0, (render_mode_ == RenderMode::DBUG) ? 1 : 0);
			glDisable(GL_BLEND);
			if (render_mode_ == RenderMode::DBUG) grid_.onFrame();
        });
    }

    void ApplicationNodeImplementation::Draw2D(FrameBuffer& fbo)
    {
        fbo.DrawToFBO([&]() {
#ifdef VISCOM_CLIENTGUI
            ImGui::ShowTestWindow();
#endif
			if (ImGui::Begin("Roomgame Controls")) {
				//ImGui::SetWindowFontScale(2.0f);
				ImGui::Text("Interaction mode: %s", (interaction_mode_==GRID)?"GRID":((interaction_mode_==GRID_PLACE_OUTER_INFLUENCE)?"GRID_PLACE_OUTER_INFLUENCE":"CAMERA"));
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
    }

    void ApplicationNodeImplementation::PostDraw()
    {
		GLenum e;
		while((e = glGetError()) != GL_NO_ERROR)
			printf("Something went wrong during the last frame (GL error %x).\n", e);
    }

    void ApplicationNodeImplementation::CleanUp()
    {
		grid_.cleanup();
		meshpool_.cleanup();
		cellular_automaton_.cleanup();
		delete shadowMap_;
		delete backgroundMesh_;
    }

    // ReSharper disable CppParameterNeverUsed
    void ApplicationNodeImplementation::KeyboardCallback(int key, int scancode, int action, int mods)
    {
		// Keys switch input modes
		static int mode_before_switch_to_camera = 0;
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
#ifdef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_KeyCallback(key, scancode, action, mods);
#endif
    }

    void ApplicationNodeImplementation::CharCallback(unsigned int character, int mods)
    {
#ifdef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_CharCallback(character);
#endif
    }

    void ApplicationNodeImplementation::MouseButtonCallback(int button, int action)
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
#ifdef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MouseButtonCallback(button, action, 0);
#endif
    }

    void ApplicationNodeImplementation::MousePosCallback(double x, double y)
    {
        if (mouseTest) {
            LOG(INFO) << "Mouse @(" << x << ", " << y << ")."; mouseTest = false;
        }
		if (interaction_mode_ == InteractionMode::GRID)
			grid_.onMouseMove(-1, x, y);
		else
			grid_.onMouseMove(-2, x, y);
		camera_.onMouseMove((float)x, (float)y);
#ifdef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MousePositionCallback(x, y);
#endif
    }

    void ApplicationNodeImplementation::MouseScrollCallback(double xoffset, double yoffset)
    {
		if (interaction_mode_ == InteractionMode::CAMERA) camera_.onScroll((float)yoffset);
#ifdef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_ScrollCallback(xoffset, yoffset);
#endif
    }
    // ReSharper restore CppParameterNeverUsed

    void ApplicationNodeImplementation::EncodeData()
    {
    }

    void ApplicationNodeImplementation::DecodeData()
    {
    }
}
