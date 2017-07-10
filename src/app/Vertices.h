/**
 * @file   Vertices.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.30
 *
 * @brief  Defines all vertices used in the application.
 */

#pragma once

#include <glm/glm.hpp>




namespace roomgame {

    /* Raw instance data that must stay memcpy-able */
    struct PerInstanceData {
        glm::vec3 translation = glm::vec3(0); // local translation
        GLfloat scale = 0.0f; // scale
        GLint buildState = 0; // build state (a value from GridCell::BuildState enum)
        GLint health = 0; // health points in [0, 100]
        static const void setAttribPointer() {
            GLint transLoc = 3;
            GLint scaleLoc = 4;
            GLint buildStateLoc = 5;
            GLint healthLoc = 6;
            glEnableVertexAttribArray(transLoc);
            glEnableVertexAttribArray(scaleLoc);
            glEnableVertexAttribArray(buildStateLoc);
            glEnableVertexAttribArray(healthLoc);
            size_t off = 0;
            glVertexAttribPointer(transLoc, 3, GL_FLOAT, false, sizeof(PerInstanceData), (GLvoid*)off);
            off += 3 * sizeof(GLfloat);
            glVertexAttribPointer(scaleLoc, 1, GL_FLOAT, false, sizeof(PerInstanceData), (GLvoid*)off);
            off += sizeof(GLfloat);
            glVertexAttribIPointer(buildStateLoc, 1, GL_INT, sizeof(PerInstanceData), (GLvoid*)off);
            off += sizeof(GLint);
            glVertexAttribIPointer(healthLoc, 1, GL_INT, sizeof(PerInstanceData), (GLvoid*)off);
            glVertexAttribDivisor(transLoc, 1);
            glVertexAttribDivisor(scaleLoc, 1);
            glVertexAttribDivisor(buildStateLoc, 1);
            glVertexAttribDivisor(healthLoc, 1);
        }
        static const void updateHealth(GLuint buffer, int offset_instances, int h) {
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            glBufferSubData(GL_ARRAY_BUFFER,
                (offset_instances + 1) * sizeof(PerInstanceData) - sizeof(health),
                sizeof(health), &h);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    };

    /* Representation of an instance buffer supporting dynamic reallocations */
    struct InstanceBuffer {
        GLuint id_; // GL handle
        int num_instances_; // current size
        size_t pool_allocation_bytes_; // chunk size for reallocations
        size_t num_reallocations_; // current number of reallocations
        InstanceBuffer() :
            pool_allocation_bytes_(0),
            num_instances_(0),
            num_reallocations_(1)
        {
            // constructor immediately pre-allocates a chunk of gpu memory
            glGenBuffers(1, &id_);
            glBindBuffer(GL_ARRAY_BUFFER, id_);
            glBufferData(GL_ARRAY_BUFFER, pool_allocation_bytes_, (GLvoid*)0, GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        InstanceBuffer(size_t pool_allocation_bytes) :
            pool_allocation_bytes_(pool_allocation_bytes),
            num_instances_(0),
            num_reallocations_(1)
        {
            // constructor immediately pre-allocates a chunk of gpu memory
            glGenBuffers(1, &id_);
            glBindBuffer(GL_ARRAY_BUFFER, id_);
            glBufferData(GL_ARRAY_BUFFER, pool_allocation_bytes_, (GLvoid*)0, GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        roomgame::InstanceBuffer& operator =(const roomgame::InstanceBuffer& other) {
            id_ = other.id_; num_instances_ = other.num_instances_;
            pool_allocation_bytes_ = other.pool_allocation_bytes_;
            num_reallocations_ = other.num_reallocations_;
            return *this;
        }
    };

}




namespace viscom {

    struct GridVertex
    {
        glm::vec3 position_;
        glm::vec4 color_;

        GridVertex(const glm::vec3& pos, const glm::vec4& col) : position_(pos), color_(col) {}
    };

    struct SimpleMeshVertex
    {
        glm::vec3 position_;
        glm::vec3 normal_;
        glm::vec2 texCoords_;

        SimpleMeshVertex() : position_(0.0f), normal_(0.0f), texCoords_(0.0f) {}
        SimpleMeshVertex(const glm::vec3& pos, const glm::vec3& normal, const glm::vec2& tex) : position_(pos), normal_(normal), texCoords_(tex) {}
        static void SetVertexAttributes(const GPUProgram* program)
        {
            auto attribLoc = program->getAttributeLocations({ "position", "normal", "texCoords" });
			glEnableVertexAttribArray(attribLoc[0]);
			glVertexAttribPointer(attribLoc[0], 3, GL_FLOAT, GL_FALSE, sizeof(SimpleMeshVertex), reinterpret_cast<GLvoid*>(offsetof(SimpleMeshVertex, position_)));
			glVertexAttribDivisor(attribLoc[0], 0);
			glEnableVertexAttribArray(attribLoc[1]);
			glVertexAttribPointer(attribLoc[1], 3, GL_FLOAT, GL_FALSE, sizeof(SimpleMeshVertex), reinterpret_cast<GLvoid*>(offsetof(SimpleMeshVertex, normal_)));
			glVertexAttribDivisor(attribLoc[1], 0);
			glEnableVertexAttribArray(attribLoc[2]);
			glVertexAttribPointer(attribLoc[2], 2, GL_FLOAT, GL_FALSE, sizeof(SimpleMeshVertex), reinterpret_cast<GLvoid*>(offsetof(SimpleMeshVertex, texCoords_)));
			glVertexAttribDivisor(attribLoc[2], 0);
        }

        static GLuint CreateVertexBuffer(const Mesh* mesh)
        {
            GLuint vbo = 0;
            glGenBuffers(1, &vbo);
            std::vector<SimpleMeshVertex> bufferMem(mesh->GetVertices().size());
            for (auto i = 0U; i < mesh->GetVertices().size(); ++i) {
                bufferMem[i].position_ = mesh->GetVertices()[i];
                bufferMem[i].normal_ = mesh->GetNormals()[i];
                bufferMem[i].texCoords_ = glm::vec2(mesh->GetTexCoords(0)[i]);
            }
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, bufferMem.size() * sizeof(SimpleMeshVertex), bufferMem.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            return vbo;
        }
    };
}
