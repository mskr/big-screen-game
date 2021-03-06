/**
 * @file   Resource.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.12.14
 *
 * @brief  Declaration of the base class for all managed resources.
 */

#pragma once

#include "main.h"

namespace viscom {

    class ApplicationNode;

    class Resource
    {
    public:
        Resource(const std::string& resourceId, ApplicationNode* appNode);
        Resource(const Resource&);
        Resource& operator=(const Resource&);
        Resource(Resource&&) noexcept;
        Resource& operator=(Resource&&) noexcept;
        virtual ~Resource();

        const std::string& GetId() const { return id_; }

    protected:
        const ApplicationNode* GetAppNode() const { return appNode_; }
        ApplicationNode* GetAppNode() { return appNode_; }

    private:
        /** Holds the resources id. */
        std::string id_;
        /** Holds the application object for dependencies. */
        ApplicationNode* appNode_;
    };
}
