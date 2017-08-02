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
		//updateManager_.ManageUpdates(deltaTime, false);
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
		outerInfluence_->meshComponent->decode();
        meshpool_.decode();
		sgct::SharedData::instance()->readObj<glm::vec3>(&synchronized_grid_translation_);
		sgct::SharedData::instance()->readFloat(&synchronized_automaton_transition_time_delta_);
		sgct::SharedData::instance()->readVector(&synchronized_grid_state_);
	}

	/* Sync step 2: Slaves set their copies of cluster-wide variables to values received from master 
	 * These copies are good because in contrast to SGCT shared objects they are not mutex locked on each access
	*/
	void SlaveNode::UpdateSyncedInfo() {
		SlaveNodeInternal::UpdateSyncedInfo();
		outerInfluence_->meshComponent->updateSyncedSlave();
        meshpool_.updateSyncedSlave();
		grid_translation_ = synchronized_grid_translation_.getVal();
		automaton_transition_time_delta_ = synchronized_automaton_transition_time_delta_.getVal();
		// GPU data:
		glBindTexture(GL_TEXTURE_2D, last_grid_state_texture_.id); // upload old grid state
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)GRID_COLS_, (GLsizei)GRID_ROWS_,
			last_grid_state_texture_.format, last_grid_state_texture_.datatype, grid_state_.data());
		grid_state_ = synchronized_grid_state_.getVal(); // fetch new grid state
		glBindTexture(GL_TEXTURE_2D, current_grid_state_texture_.id); // upload new grid state
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, (GLsizei)GRID_COLS_, (GLsizei)GRID_ROWS_,
			current_grid_state_texture_.format, current_grid_state_texture_.datatype, grid_state_.data());
	}

}
