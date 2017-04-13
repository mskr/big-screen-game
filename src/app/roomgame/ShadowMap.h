#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include "core/gfx/FrameBuffer.h"

struct ShadowMap : public viscom::FrameBuffer {
	glm::mat4 light_matrix_;
	ShadowMap(unsigned int w, unsigned int h) :
		FrameBuffer(w, h, { { viscom::FrameBufferTextureDescriptor(GL_DEPTH_COMPONENT32F) },{} }) {}
	GLuint get() const {
		return textures_[0];
	}
};

#endif