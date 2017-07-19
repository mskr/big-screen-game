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
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace viscom {

	ApplicationNodeImplementation::ApplicationNodeImplementation(ApplicationNodeInternal* appNode) :
		ApplicationNodeBase{ appNode },
		GRID_COLS_(64), GRID_ROWS_(64), GRID_HEIGHT_NDC_(2.0f),
		meshpool_(GRID_COLS_ * GRID_ROWS_),
		render_mode_(NORMAL),
		clock_{ 0.0 },
		camera_matrix_(1.0f),
		updateManager_()
    {
		outerInfluence_ = std::make_shared<roomgame::OuterInfluence>();
    }

    ApplicationNodeImplementation::~ApplicationNodeImplementation() = default;

    void ApplicationNodeImplementation::InitOpenGL()
    {
		/* Load resources on all nodes */
		meshpool_.loadShader(GetApplication()->GetGPUProgramManager());
		meshpool_.addMesh({ GridCell::INSIDE_ROOM },
                            GetApplication()->GetMeshManager().GetResource("/models/roomgame_models/floor.obj"));
		meshpool_.addMesh({ GridCell::CORNER,
							GridCell::INVALID },
                            GetApplication()->GetMeshManager().GetResource("/models/roomgame_models/corner.obj"));
		meshpool_.addMesh({ GridCell::WALL,},
                            GetApplication()->GetMeshManager().GetResource("/models/roomgame_models/wall.obj"));
		meshpool_.addMesh({ GridCell::INFECTED },
			GetApplication()->GetMeshManager().GetResource("/models/roomgame_models/latticeplane.obj"));

		SynchronizedGameMesh* outerInfluenceMeshComp = new SynchronizedGameMesh(GetApplication()->GetMeshManager().GetResource("/models/roomgame_models/latticeplane.obj"), GetApplication()->GetGPUProgramManager().GetResource("stuff",
			std::initializer_list<std::string>{ "applyTextureAndShadow.vert", "OuterInfl.frag" }));
		outerInfluence_->meshComponent = outerInfluenceMeshComp;
        glm::mat4 movMat = glm::mat4(1);
        movMat = glm::scale(movMat, glm::vec3(0.1, 0.1, 0.1));
        movMat = glm::translate(movMat, glm::vec3(0, 0, 2));
        movMat = glm::translate(movMat, glm::vec3(10, 0, 0));
        outerInfluence_->meshComponent->model_matrix_ = movMat;


		meshpool_.updateUniformEveryFrame("t_sec", [this](GLint uloc) {
			glUniform1f(uloc, (float)clock_.t_in_sec);
		});

		
		//backgroundMesh_ = new ShadowReceivingMesh(
        //    GetApplication()->GetMeshManager().GetResource("/models/roomgame_models/textured_4vertexplane/textured_4vertexplane.obj"),
        //    GetApplication()->GetGPUProgramManager().GetResource("applyTextureAndShadow",
		//		std::initializer_list<std::string>{ "applyTextureAndShadow.vert", "applyTextureAndShadow.frag" }));
		waterMesh_ = new PostProcessingMesh(
			GetApplication()->GetMeshManager().GetResource("/models/roomgame_models/textured_4vertexplane/textured_4vertexplane.obj"),
			GetApplication()->GetGPUProgramManager().GetResource("underwater",
				std::initializer_list<std::string>{ "underwater.vert", "underwater.frag" }));
		
		//backgroundMesh_->transform(glm::scale(glm::translate(glm::mat4(1), 
		waterMesh_->transform(glm::scale(glm::translate(glm::mat4(1),
			glm::vec3(
				0,
				-(GRID_HEIGHT_NDC_/GRID_ROWS_), /* position background mesh exactly under grid */
				-0.001f/*TODO better remove the z bias and use thicker meshes*/)), 
			glm::vec3(1.0f)));

		shadowMap_ = new ShadowMap(1024, 1024);
		shadowMap_->setLightMatrix(glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0), glm::vec3(0, 1, 0)));
        GetApplication()->GetEngine()->setNearAndFarClippingPlanes(0.1f, 100.0f);

		/*Set Up the camera*/
		GetCamera()->SetPosition(glm::vec3(0, 0, 0));

		updateManager_.AddUpdateable(outerInfluence_);
    }


    void ApplicationNodeImplementation::UpdateFrame(double currentTime, double elapsedTime)
    {
		deltaTime = min(currentTime - oldTime,0.25);
		clock_.t_in_sec = currentTime;
		oldTime = currentTime;
		waterMesh_->setTime(currentTime);
    }

    void ApplicationNodeImplementation::ClearBuffer(FrameBuffer& fbo)
    {
        fbo.DrawToFBO([]() {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        });

		shadowMap_->DrawToFBO([&]() {
			glClearDepth(1.0f);
			glClear(GL_DEPTH_BUFFER_BIT);
		});
    }

    void ApplicationNodeImplementation::DrawFrame(FrameBuffer& fbo)
    {
		//TODO Before the first draw there is already a framebuffer error (but seems to work anyway so far)
		//GLenum e;
		//e = glGetError(); printf("%x\n", e);

		glm::mat4 viewProj = GetCamera()->GetViewPerspectiveMatrix();

        //TODO Is the engine matrix really needed here?
		glm::mat4 lightspace = shadowMap_->getLightMatrix();

		shadowMap_->DrawToFBO([&]() {
			meshpool_.renderAllMeshesExcept(lightspace, GridCell::OUTER_INFLUENCE, 1);
		});
		

        fbo.DrawToFBO([&]() {
			//backgroundMesh_->render(viewProj, lightspace, shadowMap_->get(), (render_mode_ == RenderMode::DBG) ? 1 : 0);
			waterMesh_->render(viewProj, lightspace, shadowMap_->get(), (render_mode_ == RenderMode::DBG) ? 1 : 0);
			glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			meshpool_.renderAllMeshes(viewProj, 0, (render_mode_ == RenderMode::DBG) ? 1 : 0);
			outerInfluence_->meshComponent->render(viewProj);
			glDisable(GL_BLEND);
        });
    }

	void ApplicationNodeImplementation::PostDraw() {
		GLenum e;
		while ((e = glGetError()) != GL_NO_ERROR) {
			if (e == last_glerror_) return;
			last_glerror_ = e;
			printf("Something went wrong during the last frame (GL error %x).\n", e);
		}
	}


    void ApplicationNodeImplementation::CleanUp()
    {
		meshpool_.cleanup();
		delete shadowMap_;
		//delete backgroundMesh_;
		delete waterMesh_;
    }

    bool ApplicationNodeImplementation::KeyboardCallback(int key, int scancode, int action, int mods)
    {
        return false;
    }
}
