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

namespace viscom {

	ApplicationNodeImplementation::ApplicationNodeImplementation(ApplicationNode* appNode) :
		appNode_{ appNode },
		GRID_COLS_(64), GRID_ROWS_(64), GRID_HEIGHT_NDC_(2.0f),
		meshpool_(GRID_COLS_ * GRID_ROWS_),
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
		/* Load resources on all nodes */
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

		backgroundMesh_ = new ShadowReceivingMesh(
			appNode_->GetMeshManager().GetResource("/models/roomgame_models/textured_4vertexplane/textured_4vertexplane.obj"),
			appNode_->GetGPUProgramManager().GetResource("applyTextureAndShadow",
				std::initializer_list<std::string>{ "applyTextureAndShadow.vert", "applyTextureAndShadow.frag" }));
		backgroundMesh_->transform(glm::scale(glm::translate(glm::mat4(1), 
			glm::vec3(
				0,
				-(GRID_HEIGHT_NDC_/GRID_ROWS_), /* position background mesh exactly under grid */
				-0.001f/*TODO better remove the z bias and use thicker meshes*/)), 
			glm::vec3(1.0f)));

		shadowMap_ = new ShadowMap(1024, 1024);
		shadowMap_->setLightMatrix(glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0), glm::vec3(0, 1, 0)));
		
		GetEngine()->setNearAndFarClippingPlanes(0.1f, 100.0f);
    }

    void ApplicationNodeImplementation::PreSync()
    {
    }

    void ApplicationNodeImplementation::UpdateSyncedInfo()
    {
		//TODO Receive camera matrix
    }

    void ApplicationNodeImplementation::UpdateFrame(double currentTime, double elapsedTime)
    {
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
		glm::mat4 proj = GetEngine()->getCurrentModelViewProjectionMatrix() * camera_matrix_;

        //TODO Is the engine matrix really needed here?
		glm::mat4 lightspace = GetEngine()->getCurrentModelViewProjectionMatrix() * shadowMap_->getLightMatrix();
		
		shadowMap_->DrawToFBO([&]() {
			meshpool_.renderAllMeshesExcept(lightspace, GridCell::BuildState::OUTER_INFLUENCE, 1);
		});
		
        fbo.DrawToFBO([&]() {
			backgroundMesh_->render(proj, lightspace, shadowMap_->get(), (render_mode_ == RenderMode::DBUG) ? 1 : 0);
			glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			meshpool_.renderAllMeshes(proj, 0, (render_mode_ == RenderMode::DBUG) ? 1 : 0);
			glDisable(GL_BLEND);
        });
    }

    void ApplicationNodeImplementation::Draw2D(FrameBuffer& fbo)
    {
        fbo.DrawToFBO([&]() {
#ifdef VISCOM_CLIENTGUI
            ImGui::ShowTestWindow();
#endif
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
		meshpool_.cleanup();
		delete shadowMap_;
		delete backgroundMesh_;
    }

    // ReSharper disable CppParameterNeverUsed
    void ApplicationNodeImplementation::KeyboardCallback(int key, int scancode, int action, int mods)
    {
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
#ifdef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MouseButtonCallback(button, action, 0);
#endif
    }

    void ApplicationNodeImplementation::MousePosCallback(double x, double y)
    {
        if (mouseTest) {
            LOG(INFO) << "Mouse @(" << x << ", " << y << ")."; mouseTest = false;
        }
#ifdef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MousePositionCallback(x, y);
#endif
    }

    void ApplicationNodeImplementation::MouseScrollCallback(double xoffset, double yoffset)
    {
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
