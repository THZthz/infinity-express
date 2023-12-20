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

#include "VG.hpp"

namespace ie {

class Shader
{
public:
	Shader(const char* vertex, const char* fragment, bool isPath = false);
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

#endif // NANOVG_GL_UTILS_H

#ifdef NANOVG_GLU_IMPLEMENTATION




#undef NANOVG_GLU_IMPLEMENTATION
#endif // NANOVG_GLU_IMPLEMENTATION
