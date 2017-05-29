/**
 * @file   SlaveNode.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Declaration of the ApplicationNodeImplementation for the slave node.
 */

#pragma once

#include "core/SlaveNodeInternal.h"

namespace viscom {

    class SlaveNode final : public SlaveNodeInternal
    {

		/* camera matrix is shared by master node */
		sgct::SharedObject<glm::mat4> shared_camera_matrix_;

    public:

		/* Slave nodes fetch the data shared by master node here */
		void DecodeData() override;
		void UpdateSyncedInfo() override;

        explicit SlaveNode(ApplicationNode* appNode);
        virtual ~SlaveNode() override;

        void Draw2D(FrameBuffer& fbo) override;

    };
}
