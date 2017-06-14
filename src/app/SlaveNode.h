/**
 * @file   SlaveNode.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Declaration of the ApplicationNodeImplementation for the slave node.
 */

#pragma once

#include "core/SlaveNodeHelper.h"

namespace viscom {

    class SlaveNode final : public SlaveNodeInternal
    {

		/* camera matrix is shared by master node */
		sgct::SharedObject<glm::mat4> shared_camera_matrix_;

    public:

        explicit SlaveNode(ApplicationNodeInternal* appNode);
        virtual ~SlaveNode() override;
		void DecodeData();
		void UpdateSyncedInfo();
        void Draw2D(FrameBuffer& fbo) override;

    };
}
