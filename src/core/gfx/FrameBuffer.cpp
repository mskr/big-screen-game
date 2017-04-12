/**
 * @file   FrameBuffer.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.12.21
 *
 * @brief  Implementation of the frame buffer class.
 */

#include "FrameBuffer.h"

namespace viscom {
    /**
     * Constructor.
     * Creates a FrameBuffer representing the backbuffer.
     */
    FrameBuffer::FrameBuffer() :
        fbo_(0),
        isBackbuffer_(true),
        desc_(),
        width_(0),
        height_(0)
    {
    }

    /**
     * Constructor.
     * Creates a new FrameBuffer with given width and height. It is initialized as backbuffer as default.
     * @param fbWidth the frame buffers width
     * @param fbHeight the frame buffers height.
     * @param d the frame buffers description.
     */
    FrameBuffer::FrameBuffer(unsigned int fbWidth, unsigned int fbHeight, const FrameBufferDescriptor& desc) :
        fbo_(0),
        isBackbuffer_(false),
        desc_(desc),
        width_(0),
        height_(0)
    {
        for (auto& texDesc : desc_.texDesc_) {
            if (texDesc.texType_ == GL_TEXTURE_2D && desc_.numSamples_ != 1) texDesc.texType_ = GL_TEXTURE_2D_MULTISAMPLE;
        }
        Resize(fbWidth, fbHeight);
    }

    /**
     *  Copy constructor for a frame buffer.
     *  @param orig the original frame buffer.
     */
    FrameBuffer::FrameBuffer(const FrameBuffer& orig) :
        fbo_(0),
        isBackbuffer_(orig.isBackbuffer_),
        desc_(orig.desc_),
        width_(0),
        height_(0)
    {
        Resize(orig.width_, orig.height_);
    }

    /**
     *  Move-Constructs a frame buffer object.
     *  @param orig the original frame buffer.
     */
    FrameBuffer::FrameBuffer(FrameBuffer&& orig) noexcept :
        fbo_(orig.fbo_),
        isBackbuffer_(orig.isBackbuffer_),
        desc_(std::move(orig.desc_)),
        textures_(std::move(orig.textures_)),
        drawBuffers_(std::move(orig.drawBuffers_)),
        renderBuffers_(std::move(orig.renderBuffers_)),
        standardViewport_(orig.standardViewport_),
        width_(orig.width_),
        height_(orig.height_)
    {
        orig.fbo_ = 0;
        orig.desc_ = FrameBufferDescriptor();
    }

    /**
     *  Assigns a copy of another frame buffer.
     *  @param orig the original frame buffer.
     */
    FrameBuffer& FrameBuffer::operator=(const FrameBuffer& orig)
    {
        if (this != &orig) {
            FrameBuffer tmp{ orig };
            std::swap(*this, tmp);
        }
        return *this;
    }

    /**
     *  Assigns another frame buffer by moving its contents.
     *  @param orig the original frame buffer.
     */
    FrameBuffer& FrameBuffer::operator=(FrameBuffer&& orig) noexcept
    {
        if (this != &orig) {
            this->~FrameBuffer();
            fbo_ = orig.fbo_;
            orig.fbo_ = 0;
            isBackbuffer_ = orig.isBackbuffer_;
            desc_ = orig.desc_;
            orig.desc_ = FrameBufferDescriptor();
            textures_ = std::move(orig.textures_);
            drawBuffers_ = std::move(orig.drawBuffers_);
            renderBuffers_ = std::move(orig.renderBuffers_);
            standardViewport_ = orig.standardViewport_;
            width_ = orig.width_;
            height_ = orig.height_;
        }
        return *this;
    }

    /**
     *  Destructor.
     */
    FrameBuffer::~FrameBuffer()
    {
        glDeleteTextures(static_cast<GLsizei>(textures_.size()), textures_.data());
        textures_.clear();
        glDeleteRenderbuffers(static_cast<GLsizei>(renderBuffers_.size()), renderBuffers_.data());
        renderBuffers_.clear();
        if (fbo_ != 0) glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }

    /**
     * Resizes the frame buffer and re-initializes it if needed.
     * @param fbWidth the new width
     * @param fbHeight the new height
     */
    void FrameBuffer::Resize(unsigned int fbWidth, unsigned int fbHeight)
    {
        if (width_ == fbWidth && height_ == fbHeight) return;
        width_ = fbWidth;
        height_ = fbHeight;
        standardViewport_.size_ = glm::ivec2(width_, height_);

        if (isBackbuffer_) return;

        glGenFramebuffers(1, &fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        unsigned int colorAtt = 0;
        drawBuffers_.clear();
        glDeleteTextures(static_cast<GLsizei>(textures_.size()), textures_.data());
        textures_.clear();
        textures_.resize(desc_.texDesc_.size(), 0);
        glGenTextures(static_cast<GLsizei>(textures_.size()), textures_.data());
        for (auto i = 0U; i < textures_.size(); ++i) {
            glBindTexture(desc_.texDesc_[i].texType_, textures_[i]);
            if (desc_.texDesc_[i].texType_ == GL_TEXTURE_CUBE_MAP) {
                for (auto i = 0; i < 6; ++i) {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, desc_.texDesc_[i].internalFormat_, width_, height_, 0, GL_RGBA, GL_FLOAT, nullptr);
                }
            } else {
                if (desc_.numSamples_ == 1) {
					GLenum baseFormat = determineTextureBaseFormatFromInternalFormat(desc_.texDesc_[i].internalFormat_);
					glTexImage2D(desc_.texDesc_[i].texType_, 0, desc_.texDesc_[i].internalFormat_, width_, height_, 0, baseFormat, GL_FLOAT, nullptr);
				}
                else { 
					glTexImage2DMultisample(desc_.texDesc_[i].texType_, desc_.numSamples_, desc_.texDesc_[i].internalFormat_, width_, height_, GL_TRUE); 
				}
            }

            if (desc_.texDesc_[i].texType_ == GL_TEXTURE_CUBE_MAP) {
                for (auto i = 0; i < 6; ++i) {
                    auto attachment = findAttachment(desc_.texDesc_[i].internalFormat_, colorAtt, drawBuffers_);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, textures_[i], 0);
                }
            } else {
                auto attachment = findAttachment(desc_.texDesc_[i].internalFormat_, colorAtt, drawBuffers_);
				auto t = textures_[i];
                glFramebufferTexture(GL_FRAMEBUFFER, attachment, t, 0);
            }
        }

        glDeleteRenderbuffers(static_cast<GLsizei>(renderBuffers_.size()), renderBuffers_.data());
        renderBuffers_.clear();
        renderBuffers_.resize(desc_.rbDesc_.size(), 0);
        glGenRenderbuffers(static_cast<GLsizei>(renderBuffers_.size()), renderBuffers_.data());
        for (auto i = 0U; i < renderBuffers_.size(); ++i) {
            glBindRenderbuffer(GL_RENDERBUFFER, renderBuffers_[i]);
            if (desc_.numSamples_ == 1) { glRenderbufferStorage(GL_RENDERBUFFER, desc_.rbDesc_[i].internalFormat_, width_, height_); }
            else { glRenderbufferStorageMultisample(GL_RENDERBUFFER, desc_.numSamples_, desc_.rbDesc_[i].internalFormat_, width_, height_); }
            auto attachment = findAttachment(desc_.rbDesc_[i].internalFormat_, colorAtt, drawBuffers_);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachment, GL_RENDERBUFFER, renderBuffers_[i]);
        }

		if (renderBuffers_.size() == 0 && desc_.texDesc_.size() == 1 &&
			findAttachment(desc_.texDesc_[0].internalFormat_, colorAtt, drawBuffers_) == GL_DEPTH_ATTACHMENT) {
			// If there is only one texture as depth attachment, only use the z buffer
			glDrawBuffer(GL_NONE); // Disable color buffer writing
			glReadBuffer(GL_NONE); // Disable color buffer reading
		}
		else glDrawBuffers(static_cast<GLsizei>(drawBuffers_.size()), drawBuffers_.data());

        auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
			if (fboStatus == GL_FRAMEBUFFER_UNDEFINED) {
				printf("Specified framebuffer is the default read or draw framebuffer,");
				printf("but the default framebuffer does not exist.\n");
				throw std::runtime_error("Could not create frame buffer.");
			}
			else if (fboStatus == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT) {
				printf("Any of the framebuffer attachment points are framebuffer incomplete.");
				throw std::runtime_error("Could not create frame buffer.");
			}
			else if (fboStatus == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT) {
				printf("The framebuffer does not have at least one image attached to it.");
				throw std::runtime_error("Could not create frame buffer.");
			}
			else if (fboStatus == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER) {
				printf("The value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE");
				printf("for any color attachment point(s) named by GL_DRAW_BUFFERi.\n");
				throw std::runtime_error("Could not create frame buffer.");
			}
			else if (fboStatus == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER) {
				printf("GL_READ_BUFFER is not GL_NONE and the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE");
				printf("is GL_NONE for the color attachment point named by GL_READ_BUFFER.");
				throw std::runtime_error("Could not create frame buffer.");
			}
			else if (fboStatus == GL_FRAMEBUFFER_UNSUPPORTED) {
				printf("The combination of internal formats of the attached images");
				printf("violates an implementation-dependent set of restrictions.");
				throw std::runtime_error("Could not create frame buffer.");
			}
			else throw std::runtime_error("Could not create frame buffer.");
		}
    }

    /**
     * Use this frame buffer object as target for rendering.
     */
    void FrameBuffer::UseAsRenderTarget()
    {
        if (isBackbuffer_) {
            sgct::Engine::instance()->getCurrentWindowPtr()->getFBOPtr()->bind();
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
            glDrawBuffers(static_cast<GLsizei>(drawBuffers_.size()), drawBuffers_.data());
        }
        glViewport(standardViewport_.position_.x, standardViewport_.position_.y, standardViewport_.size_.x, standardViewport_.size_.y);
        glScissor(standardViewport_.position_.x, standardViewport_.position_.y, standardViewport_.size_.x, standardViewport_.size_.y);
    }

    /**
    * Use this frame buffer object as target for rendering and select the draw buffers used.
    * @param drawBufferIndices the indices in the draw buffer to be used.
    */
    void FrameBuffer::UseAsRenderTarget(const std::vector<unsigned int> drawBufferIndices)
    {
        assert(!isBackbuffer_);
        std::vector<GLenum> drawBuffersReduced(drawBuffers_.size());
        for (unsigned int i = 0; i < drawBufferIndices.size(); ++i) drawBuffersReduced[i] = drawBuffers_[drawBufferIndices[i]];

        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glDrawBuffers(static_cast<GLsizei>(drawBuffersReduced.size()), drawBuffersReduced.data());
        glViewport(standardViewport_.position_.x, standardViewport_.position_.y, standardViewport_.size_.x, standardViewport_.size_.y);
        glScissor(standardViewport_.position_.x, standardViewport_.position_.y, standardViewport_.size_.x, standardViewport_.size_.y);
    }

    void FrameBuffer::DrawToFBO(std::function<void()> drawFn)
    {
        UseAsRenderTarget();
        drawFn();
    }

    void FrameBuffer::DrawToFBO(const std::vector<unsigned>& drawBufferIndices, std::function<void()> drawFn)
    {
        UseAsRenderTarget(drawBufferIndices);
        drawFn();
    }

    unsigned int FrameBuffer::findAttachment(GLenum internalFormat, unsigned int& colorAtt, std::vector<GLenum> &drawBuffers)
    {
        GLenum attachment;
        switch (internalFormat)
        {
        case GL_DEPTH_STENCIL:
        case GL_DEPTH24_STENCIL8:
        case GL_DEPTH32F_STENCIL8:
            attachment = GL_DEPTH_STENCIL_ATTACHMENT;
            break;
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_COMPONENT16:
        case GL_DEPTH_COMPONENT32:
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32F:
            attachment = GL_DEPTH_ATTACHMENT;
            break;
        case GL_STENCIL_INDEX:
        case GL_STENCIL_INDEX1:
        case GL_STENCIL_INDEX4:
        case GL_STENCIL_INDEX8:
        case GL_STENCIL_INDEX16:
            attachment = GL_STENCIL_ATTACHMENT;
            break;
        default:
            attachment = GL_COLOR_ATTACHMENT0 + colorAtt++;
            drawBuffers.emplace_back(attachment);
            break;
        }
        return attachment;
    }


	GLenum FrameBuffer::determineTextureBaseFormatFromInternalFormat(GLenum internalFormat) {
		switch (internalFormat) {
		case GL_DEPTH_STENCIL:
		case GL_DEPTH24_STENCIL8:
		case GL_DEPTH32F_STENCIL8:
			return GL_DEPTH_STENCIL;
		case GL_DEPTH_COMPONENT:
		case GL_DEPTH_COMPONENT16:
		case GL_DEPTH_COMPONENT32:
		case GL_DEPTH_COMPONENT24:
		case GL_DEPTH_COMPONENT32F:
			return GL_DEPTH_COMPONENT;
		case GL_STENCIL_INDEX:
		case GL_STENCIL_INDEX1:
		case GL_STENCIL_INDEX4:
		case GL_STENCIL_INDEX8:
		case GL_STENCIL_INDEX16:
			return GL_STENCIL_INDEX;
		default:
			return GL_RGBA;
			// Note that there are countless other sized internal formats for color
			// that correspond to one of these base internal formats:
			// GL_RED, GL_RG, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA, GL_RED_INTEGER,
			// GL_RG_INTEGER, GL_RGB_INTEGER, GL_BGR_INTEGER, GL_RGBA_INTEGER, GL_BGRA_INTEGER
			// (If accounting for them, we would need also determine the corresponding data type)
		}
	}

}
