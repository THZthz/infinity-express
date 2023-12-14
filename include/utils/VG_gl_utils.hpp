//
// Copyright (c) 2020 Stylus Labs - see LICENSE.txt
//   based on nanovg:
// Copyright (c) 2013 Mikko Mononen memon@inside.org
//
#ifndef NANOVG_GL_UTILS_H
#define NANOVG_GL_UTILS_H

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace ie {

class Shader
{
public:
	Shader(const char* vertex, const char* fragment);
	~Shader();

	unsigned int getProgram() const;
	void deleteShader();

private:
	unsigned int m_vertexShader = 0, m_fragmentShader = 0;
	unsigned int m_shaderProgram = 0;
};

// we'll assume FBO functionality is available (as nanovg-2 doesn't work without it)
class Framebuffer
{
public:
	enum Flags
	{
		NO_NVG_IMAGE = 1 << 24, // do not create a nanovg image for the texture
	};

	Framebuffer(NVGcontext* ctx, int w, int h, int imageFlags);
	~Framebuffer();

	int bind() const;
	static void setSRGB(bool enable);
	void setSize(int w, int h, int imageFlags);
	int getImageHandle() const;
	unsigned int getInternalHandle() const;
	void blit(int destFBO) const;
	void readPixels(void* dest) const;

private:
	static constexpr GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};

	NVGcontext* m_ctx;
	GLuint m_fbo = 0;
	GLuint m_texture = 0;
	int m_image = 0;
	int m_width = 0;
	int m_height = 0;
};

// use shader to directly copy texture to texture
class TextureCopy
{
public:
	void
	copy(GLuint dst, GLint dst_w, GLint dst_h, GLuint src, GLuint src_w, GLuint src_h) const;

private:
	static constexpr char* vertex =(char *const) "";
	static constexpr char* fragment =(char *const) "";

	Shader m_shader{vertex, fragment};
};

} // namespace ie

namespace ie {

inline Framebuffer::Framebuffer(NVGcontext* ctx, int w, int h, int imageFlags) : m_ctx(ctx)
{
	GLint defaultFBO;
	//GLint defaultRBO;

	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &defaultFBO);
	//glGetIntegerv(GL_RENDERBUFFER_BINDING, &defaultRBO);

	// frame buffer object
	glGenFramebuffers(1, &m_fbo);

	if (imageFlags & NO_NVG_IMAGE)
	{
		m_image = -1;
		if (w <= 0 || h <= 0) return;

		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		setSize(w, h, imageFlags);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
		m_width = w;
		m_height = h;
		m_image = nvgCreateImageRGBA(
		    ctx, w, h, imageFlags | NVG_IMAGE_FLIPY | NVG_IMAGE_PREMULTIPLIED, nullptr);
		m_texture = nvglImageHandle(ctx, m_image);
		glFramebufferTexture2D(
		    GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
	}
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		if (m_fbo != 0) glDeleteFramebuffers(1, &m_fbo);
		if (m_image >= 0) nvgDeleteImage(m_ctx, m_image);
		else glDeleteTextures(1, &m_texture);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, defaultFBO);
}

// returns previously bound FBO
inline int
Framebuffer::bind() const
{
	int prevFBO = -1;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	return prevFBO;
}

// enable or disable automatic sRGB conversion *if* writing to sRGB framebuffer on desktop GL; for GLES,
//  GL_FRAMEBUFFER_SRGB is not available and sRGB conversion is enabled iff framebuffer is sRGB
inline void
Framebuffer::setSRGB(bool enable)
{
	enable ? glEnable(GL_FRAMEBUFFER_SRGB) : glDisable(GL_FRAMEBUFFER_SRGB);
}

inline void
Framebuffer::setSize(int w, int h, int imageFlags)
{
	GLint format = imageFlags & NVG_IMAGE_SRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
	if (w <= 0 || h <= 0 || (w == m_width && h == m_height)) return;
	if (m_image >= 0)
	{
		throw std::runtime_error(
		    "can only be used with framebuffer created with "
		    "NO_NVG_IMAGE");
		return;
	}

	glDeleteTextures(1, &m_texture);
	glGenTextures(1, &m_texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_HALF_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
	m_width = w;
	m_height = h;
}

// assumes FBO (source) is already bound; destFBO is bounds on return
inline void
Framebuffer::blit(int destFBO) const
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destFBO);
	glBlitFramebuffer(
	    0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	//glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, 1, drawBuffers);
	glBindFramebuffer(GL_FRAMEBUFFER, destFBO);
}

inline void
Framebuffer::readPixels(void* dest) const
{
	// for desktop GL, we could use glGetTexImage
	glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, dest);
}

inline Framebuffer::~Framebuffer()
{
	if (m_fbo != 0) glDeleteFramebuffers(1, &m_fbo);
	if (m_image >= 0) nvgDeleteImage(m_ctx, m_image);
	else glDeleteTextures(1, &m_texture);
	m_ctx = nullptr;
	m_fbo = 0;
	m_texture = 0;
	m_image = -1;
}

inline int
Framebuffer::getImageHandle() const
{
	return m_image;
}

inline unsigned int
Framebuffer::getInternalHandle() const
{
	return m_texture;
}

inline Shader::Shader(const char* vertex, const char* fragment)
{
	char info[512];
	int success;

	// vertex shader
	m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(m_vertexShader, 1, &vertex, nullptr);
	glCompileShader(m_vertexShader);
	// check for shader compile errors
	glGetShaderiv(m_vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(m_vertexShader, 512, nullptr, info);
		printf("%s\n", ("cannot compile vertex shader" + std::string(info)).c_str());
	}

	// fragment shader
	m_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(m_fragmentShader, 1, &fragment, nullptr);
	glCompileShader(m_fragmentShader);
	// check for shader compile errors
	glGetShaderiv(m_fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(m_fragmentShader, 512, nullptr, info);
		printf("%s\n", ("cannot compile fragment shader" + std::string(info)).c_str());
	}

	// link shaders
	m_shaderProgram = glCreateProgram();
	glAttachShader(m_shaderProgram, m_vertexShader);
	glAttachShader(m_shaderProgram, m_fragmentShader);
	glLinkProgram(m_shaderProgram);
	// check for linking errors
	glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(m_shaderProgram, 512, nullptr, info);
		printf("%s\n", ("cannot link shader" + std::string(info)).c_str());
	}
}

inline void
Shader::deleteShader()
{
	glDeleteShader(m_vertexShader); // TODO: dont delete shader twice
	glDeleteShader(m_fragmentShader);
	m_vertexShader = 0;
	m_fragmentShader = 0;
}

inline Shader::~Shader() { deleteShader(); }

inline unsigned int
Shader::getProgram() const
{
	return m_shaderProgram;
}

inline void
TextureCopy::copy(GLuint dst, GLint dst_w, GLint dst_h, GLuint src, GLuint src_w, GLuint src_h)
    const
{
}

} // namespace ie

#endif // NANOVG_GL_UTILS_H

#ifdef NANOVG_GLU_IMPLEMENTATION




#undef NANOVG_GLU_IMPLEMENTATION
#endif // NANOVG_GLU_IMPLEMENTATION
