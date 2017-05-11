/**
 * @file   ApplicationNodeImplementation.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.30
 *
 * @brief  Declaration of the application node implementation common for master and slave nodes.
 */

#pragma once

#include <sgct/Engine.h>
#include "core/ApplicationNode.h"
#include "app/roomgame/AutomatonGrid.h"
#include "app/roomgame/RoomSegmentMeshPool.h"
#include "app/roomgame/DragAndZoomCamera.h"
#include "app/roomgame/OuterInfluenceAutomaton.h"
#include "app/roomgame/GameMesh.h"
#include "app/roomgame/ShadowMap.h"

namespace viscom {

    class MeshRenderable;

    class ApplicationNodeImplementation
    {
    public:
        explicit ApplicationNodeImplementation(ApplicationNode* appNode);
        ApplicationNodeImplementation(const ApplicationNodeImplementation&) = delete;
        ApplicationNodeImplementation(ApplicationNodeImplementation&&) = delete;
        ApplicationNodeImplementation& operator=(const ApplicationNodeImplementation&) = delete;
        ApplicationNodeImplementation& operator=(ApplicationNodeImplementation&&) = delete;
        virtual ~ApplicationNodeImplementation();

        virtual void PreWindow();
        virtual void InitOpenGL();
        virtual void PreSync();
        virtual void UpdateSyncedInfo();
        virtual void UpdateFrame(double currentTime, double elapsedTime);
        virtual void ClearBuffer(FrameBuffer& fbo);
        virtual void DrawFrame(FrameBuffer& fbo);
        virtual void Draw2D(FrameBuffer& fbo);
        virtual void PostDraw();
        virtual void CleanUp();

        virtual void KeyboardCallback(int key, int scancode, int action, int mods);
        virtual void CharCallback(unsigned int character, int mods);
        virtual void MouseButtonCallback(int button, int action);
        virtual void MousePosCallback(double x, double y);
        virtual void MouseScrollCallback(double xoffset, double yoffset);

        virtual void EncodeData();
        virtual void DecodeData();

    protected:
        sgct::Engine* GetEngine() const { return appNode_->GetEngine(); }
        const FWConfiguration& GetConfig() const { return appNode_->GetConfig(); }
        ApplicationNode* GetApplication() const { return appNode_; }

        unsigned int GetGlobalProjectorId(int nodeId, int windowId) const { return appNode_->GetGlobalProjectorId(nodeId, windowId); }

        const Viewport& GetViewportScreen(size_t windowId) const { return appNode_->GetViewportScreen(windowId); }
        Viewport& GetViewportScreen(size_t windowId) { return appNode_->GetViewportScreen(windowId); }
        const glm::ivec2& GetViewportQuadSize(size_t windowId) const { return appNode_->GetViewportQuadSize(windowId); }
        glm::ivec2& GetViewportQuadSize(size_t windowId) { return appNode_->GetViewportQuadSize(windowId); }
        const glm::vec2& GetViewportScaling(size_t windowId) const { return appNode_->GetViewportScaling(windowId); }
        glm::vec2& GetViewportScaling(size_t windowId) { return appNode_->GetViewportScaling(windowId); }

        double GetCurrentAppTime() const { return appNode_->GetCurrentAppTime(); }
        double GetElapsedTime() const { return appNode_->GetElapsedTime(); }

        /** Holds the application node. */
        ApplicationNode* appNode_;

		AutomatonGrid grid_;
		RoomSegmentMeshPool meshpool_;
		DragAndZoomCamera camera_;
		enum InteractionMode { GRID, CAMERA, GRID_PLACE_OUTER_INFLUENCE } interaction_mode_;
		OuterInfluenceAutomaton cellular_automaton_;
		ShadowMap* shadowMap_;
		ShadowReceivingMesh* backgroundMesh_;
		enum RenderMode { NORMAL, DBUG } render_mode_;

		struct Clock {
			double t_in_sec;
		} clock_;

		struct Quad {
			std::shared_ptr<GPUProgram> shader;
			GLint texture_uniform_location;
			GLuint vao;
			void init(GPUProgramManager mgr) {
				glGenVertexArrays(1, &vao);
				glBindVertexArray(vao);
				GLfloat quad[] = {
					// (x, y)      // (u, v)
					-1.0f,  1.0f,  0.0f, 1.0f, // top left
					1.0f, -1.0f,  1.0f, 0.0f, // bottom right
					-1.0f, -1.0f,  0.0f, 0.0f, // bottom left
					-1.0f,  1.0f,  0.0f, 1.0f, // top left
					1.0f,  1.0f,  1.0f, 1.0f, // top right
					1.0f, -1.0f,  1.0f, 0.0f // bottom right
				};
				GLuint vbo;
				glGenBuffers(1, &vbo);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
				glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
				glEnableVertexAttribArray(0);
				glEnableVertexAttribArray(1);
				glBindVertexArray(0);
				shader = mgr.GetResource("applyTextureToQuad",
					std::initializer_list<std::string>{ "applyTextureToQuad.vert", "applyTextureToQuad.frag" });
				texture_uniform_location = shader->getUniformLocation("texture");
			}
			void render(GLuint texture) const {
				glUseProgram(shader->getProgramId());
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_CULL_FACE);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, texture);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
				glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
				glUniform1i(texture_uniform_location, 0);
				glBindVertexArray(vao);
				glDrawArrays(GL_TRIANGLES, 0, 6);
				glBindVertexArray(0);
				glEnable(GL_CULL_FACE);
				glEnable(GL_DEPTH_TEST);
			}
		} screenfilling_quad_;
	};
}
