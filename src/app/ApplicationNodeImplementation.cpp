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

#define GRID_COLUMNS 128
#define GRID_ROWS 128

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
		grid_(GRID_COLUMNS, GRID_ROWS, 1.0f, &meshpool_),
		interaction_mode_(GRID_PLACE_OUTER_INFLUENCE),
		camera_(glm::mat4(1)),
		cellular_automaton_(&grid_, automaton_transition_time)
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
							appNode_->GetMeshManager().GetResource("/models/roomgame_models/4vertexplane.obj"));

		grid_.onMeshpoolInitialized();
		grid_.loadShader(appNode_->GetGPUProgramManager());
		grid_.uploadVertexData();

		ImGui::GetIO().FontAllowUserScaling = true;
		ImGui::GetIO().FontGlobalScale = 1.5f;

        backgroundMesh_.shader = appNode_->GetGPUProgramManager().GetResource("applyTexture",
			std::initializer_list<std::string>{ "applyTexture.vert", "applyTexture.frag" });
		backgroundMesh_.view_projection_uniform_location = backgroundMesh_.shader->getUniformLocation("viewProjectionMatrix");
		backgroundMesh_.mesh_resource = appNode_->GetMeshManager().GetResource("/models/roomgame_models/textured_4vertexplane/textured_4vertexplane.obj");
		backgroundMesh_.mesh_renderable = MeshRenderable::create<SimpleMeshVertex>(backgroundMesh_.mesh_resource.get(), backgroundMesh_.shader.get());
		backgroundMesh_.model_matrix = glm::scale(glm::translate(glm::mat4(1), glm::vec3(-3.0f, 0.0f, -5.0f)), glm::vec3(2.0f));
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
    }

    void ApplicationNodeImplementation::ClearBuffer(FrameBuffer& fbo)
    {
        fbo.DrawToFBO([]() {
            auto colorPtr = sgct::Engine::instance()->getClearColor();
            glClearColor(colorPtr[0], colorPtr[1], colorPtr[2], colorPtr[3]);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        });
    }

    void ApplicationNodeImplementation::DrawFrame(FrameBuffer& fbo)
    {
		glm::mat4 proj = GetEngine()->getCurrentModelViewProjectionMatrix() * camera_.getViewProjection();
		grid_.updateProjection(proj);
        fbo.DrawToFBO([&]() {
			meshpool_.renderAllMeshes(proj);
			grid_.onFrame(); // debug render
			backgroundMesh_.render(proj);
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
		else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
			if (interaction_mode_ == GRID_PLACE_OUTER_INFLUENCE) {
				interaction_mode_ = InteractionMode::GRID;
				cellular_automaton_.init(appNode_->GetGPUProgramManager());
			}
			else {
				interaction_mode_ = GRID_PLACE_OUTER_INFLUENCE;
			}
		}
#ifdef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_KeyCallback(key, scancode, action, mods);
#endif
    }

    void ApplicationNodeImplementation::CharCallback(unsigned character, int mods)
    {
#ifdef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_CharCallback(character);
#endif
    }

    void ApplicationNodeImplementation::MouseButtonCallback(int button, int action)
    {
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (interaction_mode_ == InteractionMode::GRID) {
				if (action == GLFW_PRESS) grid_.onTouch(-1);
				else if (action == GLFW_RELEASE) grid_.onRelease(-1);
			}
			else if (interaction_mode_ == InteractionMode::GRID_PLACE_OUTER_INFLUENCE) {
				if (action == GLFW_PRESS) grid_.buildAtLastMousePosition(GridCell::BuildState::OUTER_INFLUENCE);
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
