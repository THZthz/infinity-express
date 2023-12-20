#include "utils/VG_gl_utils.hpp"
#include "utils/VG_gl.hpp"
#include "utils/filesystem.hpp"

using namespace ie;

Framebuffer::Framebuffer(NVGcontext* ctx, int w, int h, int imageFlags) : m_ctx(ctx)
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
int
Framebuffer::bind() const
{
	int prevFBO = -1;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
	return prevFBO;
}

// enable or disable automatic sRGB conversion *if* writing to sRGB framebuffer on desktop GL; for GLES,
//  GL_FRAMEBUFFER_SRGB is not available and sRGB conversion is enabled iff framebuffer is sRGB
void
Framebuffer::setSRGB(bool enable)
{
	enable ? glEnable(GL_FRAMEBUFFER_SRGB) : glDisable(GL_FRAMEBUFFER_SRGB);
}

void
Framebuffer::setSize(int w, int h, int imageFlags)
{
	GLint format = imageFlags & NVG_IMAGE_SRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
	if (w <= 0 || h <= 0 || (w == m_width && h == m_height)) return;
	if (m_image >= 0)
	{
		throw std::runtime_error(
		    "can only be used with framebuffer created with "
		    "NO_NVG_IMAGE");
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
void
Framebuffer::blit(int destFBO) const
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destFBO);
	glBlitFramebuffer(
	    0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	//glInvalidateFramebuffer(GL_READ_FRAMEBUFFER, 1, drawBuffers);
	glBindFramebuffer(GL_FRAMEBUFFER, destFBO);
}

void
Framebuffer::readPixels(void* dest) const
{
	// for desktop GL, we could use glGetTexImage
	glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, dest);
}

Framebuffer::~Framebuffer()
{
	if (m_fbo != 0) glDeleteFramebuffers(1, &m_fbo);
	if (m_image >= 0) nvgDeleteImage(m_ctx, m_image);
	else glDeleteTextures(1, &m_texture);
	m_ctx = nullptr;
	m_fbo = 0;
	m_texture = 0;
	m_image = -1;
}

int
Framebuffer::getImageHandle() const
{
	return m_image;
}

unsigned int
Framebuffer::getInternalHandle() const
{
	return m_texture;
}

Shader::Shader(const char* vertex, const char* fragment, bool isPath)
{
	char info[512];
	int success;

	char* vertexSource = const_cast<char*>(vertex);
	char* fragmentSource = const_cast<char*>(fragment);

	// load from disk if is path
	std::unique_ptr<char[]> v, f;
	if (isPath)
	{
		size_t vSize, fSize;
		v = filesystem::readFile(vertex, &vSize);
		f = filesystem::readFile(fragment, &fSize);
		assert(f && v);
		vertexSource = v.get();
		fragmentSource = f.get();
	}

	// vertex shader
	m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(m_vertexShader, 1, &vertexSource, nullptr);
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
	glShaderSource(m_fragmentShader, 1, &fragmentSource, nullptr);
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

void
Shader::deleteShader()
{
	glDeleteShader(m_vertexShader); // TODO: dont delete shader twice
	glDeleteShader(m_fragmentShader);
	m_vertexShader = 0;
	m_fragmentShader = 0;
}

Shader::~Shader() { deleteShader(); }

unsigned int
Shader::getProgram() const
{
	return m_shaderProgram;
}

void
TextureCopy::copy(GLuint dst, GLint dst_w, GLint dst_h, GLuint src, GLuint src_w, GLuint src_h)
    const
{
}
