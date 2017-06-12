/**
 * @file   MasterNode.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Declaration of the ApplicationNodeImplementation for the master node.
 */

#pragma once

#include "../app/ApplicationNodeImplementation.h"
#ifdef WITH_TUIO
#include "core/TuioInputWrapper.h"
#endif

namespace viscom {

	/* Roomgame controller class for master node
	 * Holds the grid with all the build states
	 * Uses cellular automaton to compute dynamic influence
	 * Can modify the view by using camera
	 * Handles all user input
	*/
    class MasterNode final : public ApplicationNodeImplementation
    {

	// Grid runs only on master node
	// (As soon as grid uses RoomSegmentMeshes to add instances, data is written into buffer, that is synced with slaves)
	AutomatonGrid grid_;

	// Automaton only created and running on master node
	// (As soon as automaton updates MeshInstanceGrid, data is written into buffer, that is synced with slaves)
	OuterInfluenceAutomaton cellular_automaton_;

	// Camera is controlled only on master but camera matrix must be synced
	// (As soon as matrix is passed to a render method, this render method must run on all slaves)
	DragAndZoomCamera camera_;
	sgct::SharedObject<glm::mat4> shared_camera_matrix_; // camera matrix the master shares

	/* Interaction mode only exists on master node */
	enum InteractionMode { GRID, CAMERA, GRID_PLACE_OUTER_INFLUENCE } interaction_mode_;

    public:
        explicit MasterNode(ApplicationNodeInternal* appNode);
        virtual ~MasterNode() override;

        void InitOpenGL() override;

		/* Master node sends shared data here */
        void PreSync() override;
		void EncodeData() override;
		void UpdateSyncedInfo() override;

        void DrawFrame(FrameBuffer& fbo) override;
        void Draw2D(FrameBuffer& fbo) override;

        void CleanUp() override;

        bool KeyboardCallback(int key, int scancode, int action, int mods) override;
        bool CharCallback(unsigned int character, int mods) override;
        bool MouseButtonCallback(int button, int action) override;
        bool MousePosCallback(double x, double y) override;
        bool MouseScrollCallback(double xoffset, double yoffset) override;

    };
}
