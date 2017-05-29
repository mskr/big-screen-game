/**
 * @file   SlaveNode.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Implementation of the slave application node.
 */

#include "SlaveNode.h"

namespace viscom {

    SlaveNode::SlaveNode(ApplicationNode* appNode) :
        SlaveNodeInternal{ appNode }
    {
    }

    void SlaveNode::Draw2D(FrameBuffer& fbo)
    {
        // always do this call last!
        SlaveNodeInternal::Draw2D(fbo);
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
