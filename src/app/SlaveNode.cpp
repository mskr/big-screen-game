/**
 * @file   SlaveNode.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Implementation of the slave application node.
 */

#include "SlaveNode.h"
#include <imgui.h>

namespace viscom {

    SlaveNode::SlaveNode(ApplicationNodeInternal* appNode) :
        SlaveNodeInternal{ appNode }
    {
    }


	void SlaveNode::UpdateFrame(double t1, double t2) {
		ApplicationNodeImplementation::UpdateFrame(t1, t2);
	}

    void SlaveNode::Draw2D(FrameBuffer& fbo)
    {
#ifdef VISCOM_CLIENTGUI
        ImGui::ShowTestWindow();
#endif
		updateManager_.ManageUpdates(GetApplication()->GetElapsedTime(), false);
        // always do this call last!
        SlaveNodeInternal::Draw2D(fbo);
    }

	void SlaveNode::PostDraw() {
		ApplicationNodeImplementation::PostDraw();
	}

    SlaveNode::~SlaveNode() = default;

	/* Sync step 1: Slave nodes fetch the data shared by master node here 
	 * (Order in decoding must be the same as in encoding on master)
	*/
	void SlaveNode::DecodeData() {
		SlaveNodeInternal::DecodeData();
		sgct::SharedData::instance()->readObj(&shared_camera_matrix_);
	}

	/* Sync step 2: Slaves set their copies of cluster-wide variables to values received from master */
	void SlaveNode::UpdateSyncedInfo() {
		SlaveNodeInternal::UpdateSyncedInfo();
		camera_matrix_ = shared_camera_matrix_.getVal();
	}

}
