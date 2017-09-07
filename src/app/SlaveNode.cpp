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
        outerInfluence_->MeshComponent->decode();
        meshpool_.decode();
        sgct::SharedData::instance()->readObj<glm::vec3>(&synchronized_grid_translation_);
        sgct::SharedData::instance()->readFloat(&synchronized_automaton_transition_time_delta_);
        sgct::SharedData::instance()->readBool(&synchronized_automaton_has_transitioned_);
        if(synchronized_automaton_has_transitioned_.getVal())
            sgct::SharedData::instance()->readVector(&synchronized_grid_state_);
    }

    /* Sync step 2: Slaves set their copies of cluster-wide variables to values received from master 
     * These copies are good because in contrast to SGCT shared objects they are not mutex locked on each access
    */
    void SlaveNode::UpdateSyncedInfo() {
        SlaveNodeInternal::UpdateSyncedInfo();
        outerInfluence_->MeshComponent->updateSyncedSlave();
        meshpool_.updateSyncedSlave();
        grid_translation_ = synchronized_grid_translation_.getVal();
        automaton_transition_time_delta_ = synchronized_automaton_transition_time_delta_.getVal();
        automaton_has_transitioned_ = synchronized_automaton_has_transitioned_.getVal();
        // GPU data upload behind check if GL was initialized
        if (last_grid_state_texture_.id > 0 && current_grid_state_texture_.id > 0) {
            // ensure that this happens only once after a automaton transition to have last and current state right
            if (automaton_has_transitioned_) {
                // Grid state: type UINT has to be converted to UNORM to make use of bilinear interpolation when rendering
                glBindTexture(GL_TEXTURE_2D, last_grid_state_texture_.id);
                glTexImage2D(GL_TEXTURE_2D, 0,
                    GL_RG32F, // 32 bit UNORM means 1.0F == 2^31 == 4294967296U (=> 1.0/(2^22) == 2^9 == 512U == INFECTED)
                    GRID_COLS_, GRID_ROWS_, 0,
                    GL_RG, // no "_INTEGER" postfix means data is treated as NORM and sampling delivers float
                    GL_UNSIGNED_INT, // pixel data points to integers treated as UNORM
                    grid_state_.data());
                grid_state_ = synchronized_grid_state_.getVal(); // fetch new Grid state
                glBindTexture(GL_TEXTURE_2D, current_grid_state_texture_.id);
                glTexImage2D(GL_TEXTURE_2D, 0,
                    GL_RG32F, // 32 bit UNORM means 1.0F == 2^31 == 4294967296U (=> 1.0/(2^22) == 2^9 == 512U == INFECTED)
                    GRID_COLS_, GRID_ROWS_, 0,
                    GL_RG, // no "_INTEGER" postfix means data is treated as NORM and sampling delivers float
                    GL_UNSIGNED_INT, // pixel data points to integers treated as UNORM
                    grid_state_.data());
            }
        }
    }

}
