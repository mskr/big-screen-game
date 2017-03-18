#ifndef GPUBUFFER_H_
#define GPUBUFFER_H_

#include <sgct/Engine.h>

/*
* A gpu buffer representation.
* Can be used as
* a) Vertex buffer
* b) Framebuffer
*/
class GPUBuffer {
	bool is_fbo_;
	GLuint opengl_id_;
public:
	struct Tex {
		GLuint id;
		GLenum attachmentType;
		GLint sized_format;
		GLenum format;
		GLenum datatype;
	};
	// Create multipurpose buffer
	GPUBuffer();
	GPUBuffer(GLsizeiptr bytes, const GLvoid* pointer);
	// Create framebuffer with one or more 2D textures
	GPUBuffer(GLsizei w, GLsizei h, std::initializer_list<Tex*> tex_attachments);
	~GPUBuffer();
	GLuint id() { return opengl_id_; }
	void bind();
	void bind_to(GLenum targetpoint);
	// Bind and init gpu memory
	void bind_data(GLsizeiptr bytes, const GLvoid* pointer);
	// Create empty 2D texture without mipmap
	static GLuint new_texture2D(GLsizei w, GLsizei h, GLint sized_format, GLenum format, GLenum type);

};

#endif