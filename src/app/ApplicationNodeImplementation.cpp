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

namespace viscom {

	ApplicationNodeImplementation::ApplicationNodeImplementation(ApplicationNode* appNode) :
		appNode_{ appNode },
		grid_(40, 40, 1.2f)
    {
    }


    ApplicationNodeImplementation::~ApplicationNodeImplementation() = default;

    void ApplicationNodeImplementation::PreWindow()
    {
    }

    void ApplicationNodeImplementation::InitOpenGL()
    {
		grid_.loadShader(appNode_);
		grid_.uploadVertexData();
    }

    void ApplicationNodeImplementation::PreSync()
    {
    }

    void ApplicationNodeImplementation::UpdateSyncedInfo()
    {
    }

    void ApplicationNodeImplementation::UpdateFrame(double currentTime, double)
    {
        triangleModelMatrix_ = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f)), static_cast<float>(currentTime), glm::vec3(0.0f, 1.0f, 0.0f));
        teapotModelMatrix_ = glm::scale(glm::rotate(glm::translate(glm::mat4(0.01f), glm::vec3(-3.0f, 0.0f, -5.0f)), static_cast<float>(currentTime), glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(0.01f));
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
        fbo.DrawToFBO([this]() {
			grid_.render(GetEngine()->getCurrentModelViewProjectionMatrix());
        });
    }

    void ApplicationNodeImplementation::Draw2D(FrameBuffer& fbo)
    {
        fbo.DrawToFBO([]() {
#ifdef VISCOM_CLIENTGUI
            ImGui::ShowTestWindow();
#endif
        });
    }

    void ApplicationNodeImplementation::PostDraw()
    {
    }

    void ApplicationNodeImplementation::CleanUp()
    {
		grid_.cleanup();
    }

    // ReSharper disable CppParameterNeverUsed
    void ApplicationNodeImplementation::KeyboardCallback(int key, int scancode, int action, int mods)
    {
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
#ifdef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MouseButtonCallback(button, action, 0);
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (action == GLFW_PRESS)
				grid_.onTouch(-1);
			else if (action == GLFW_RELEASE)
				grid_.onRelease(-1);
		}
#endif
    }

    void ApplicationNodeImplementation::MousePosCallback(double x, double y)
    {
#ifdef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MousePositionCallback(x, y);
		grid_.onMouseMove(-1, x, y);
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
