#include "GPUBuffer.h"
#include <iostream>

GPUBuffer::GPUBuffer() {
	glGenBuffers(1, &opengl_id_);
	is_fbo_ = false;
}

GPUBuffer::GPUBuffer(GLsizeiptr bytes, const GLvoid* pointer) {
	glGenBuffers(1, &opengl_id_);
	glBindBuffer(GL_ARRAY_BUFFER, opengl_id_);
	glBufferData(GL_ARRAY_BUFFER, bytes, pointer, GL_STATIC_DRAW);
	is_fbo_ = false;
}

GPUBuffer::GPUBuffer(GLsizei w, GLsizei h, std::initializer_list<Tex*> tex_attachments) {
	glGenFramebuffers(1, &opengl_id_);
	glBindFramebuffer(GL_FRAMEBUFFER, opengl_id_);
	int num_color_attachments = 0;
	int num_depth_attachments = 0;
	int num_stencil_attachments = 0;
	for (Tex* tex : tex_attachments) {
		GLuint tex_ptr = tex->id;
		tex->id = new_texture2D(w, h, tex->sized_format, tex->format, tex->datatype);
		GLenum attachment_type = tex->attachmentType;
        std::cout << "after 2 " << glGetError() << std::endl;

		glFramebufferTexture2D(GL_FRAMEBUFFER, attachment_type, GL_TEXTURE_2D, tex->id, 0);
        std::cout << "after 3 " << glGetError() << std::endl;
        // Count attachments
		GLint max_color_attachments = 8;
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
		if ((int)attachment_type >= (int)GL_COLOR_ATTACHMENT0 && (int)attachment_type <= ((int)GL_COLOR_ATTACHMENT0 + max_color_attachments - 1))
			num_color_attachments++;
		else if (attachment_type == GL_DEPTH_ATTACHMENT)
			num_depth_attachments++;
		else if (attachment_type == GL_STENCIL_ATTACHMENT)
			num_stencil_attachments++;
		else if (attachment_type == GL_DEPTH_STENCIL_ATTACHMENT) {
			num_depth_attachments++;
			num_stencil_attachments++;
		}
	}
	// If no color texture, disable color rendering
	if (num_color_attachments == 0) {
		glDrawBuffer(GL_NONE); // OpenGL won't try to write to a color buffer
		glReadBuffer(GL_NONE); // reading from color buffer is also disabled
	}
	else if (num_color_attachments > 1) {
		//TODO Link fragment shader out variables to attachments with glBindFragDataLocation
	}
	// If no depth and stencil texture, attach renderbuffer
	if (num_depth_attachments == 0 && num_stencil_attachments == 0) {
		// Renderbuffer is write-only, and probably faster than textures
		GLuint renderbuffer = 0;
		glGenRenderbuffers(1, &renderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);
	}
	else if (num_depth_attachments == 0) {
		//TODO do something for color+stencil attachments
	}
	else if (num_stencil_attachments == 0) {
		//TODO do something for color+depth attachments
	}
	// Check if framebuffer complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		throw std::runtime_error("Program exits because I failed to complete framebuffer.");
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	is_fbo_ = true;
}

GPUBuffer::~GPUBuffer() {
	if (is_fbo_)
		glDeleteFramebuffers(1, &opengl_id_);
	else
		glDeleteBuffers(1, &opengl_id_);
	opengl_id_ = 0;
}

void GPUBuffer::bind() {
	if (is_fbo_)
		glBindFramebuffer(GL_FRAMEBUFFER, opengl_id_);
	else
		glBindBuffer(GL_ARRAY_BUFFER, opengl_id_);
}

void GPUBuffer::bind_to(GLenum targetpoint) {
	if (is_fbo_)
		glBindFramebuffer(targetpoint, opengl_id_);
	else
		glBindBuffer(targetpoint, opengl_id_);
}

void GPUBuffer::bind_data(GLsizeiptr bytes, const GLvoid* pointer) {
	if (is_fbo_)
		return;
	glBindBuffer(GL_ARRAY_BUFFER, opengl_id_);
	glBufferData(GL_ARRAY_BUFFER, bytes, pointer, GL_STATIC_DRAW);
}

GLuint GPUBuffer::new_texture2D(GLsizei w, GLsizei h, GLint sized_format, GLenum format, GLenum type) {
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
    //glTexImage2D(GL_TEXTURE_2D, 0, sized_format, w, h, 0, format, type, NULL);

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//    GLint lala;
//    glGetIntegerv(GL_TEXTURE_BINDING_2D, &lala);
//    std::cout << lala << std::endl;
    std::cout << "before glTexStorage2D"<<glGetError() << std::endl;
    glTexStorage2D(GL_TEXTURE_2D, 1, sized_format, w, h);
    std::cout << "after  glTexStorage2D" << glGetError() << std::endl;
	return tex;
}