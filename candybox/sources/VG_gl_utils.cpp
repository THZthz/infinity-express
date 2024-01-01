#include <cstring>
#include "candybox/vg/VG_gl_utils.hpp"
#include "candybox/vg/VG_gl.hpp"
#include "candybox/filesystem.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "candybox/stb/stb_image.h"

using namespace candybox;

Framebuffer::Framebuffer(NVGcontext *ctx, int w, int h, int imageFlags) : m_ctx(ctx)
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
Framebuffer::readPixels(void *dest) const
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

Shader::Shader(const char *vertex, const char *fragment, bool isPath)
{
	char info[512];
	int success;

	char *vertexSource = const_cast<char *>(vertex);
	char *fragmentSource = const_cast<char *>(fragment);

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

SpriteBatch::Texture
SpriteBatch::createTexture(const char *filename)
{
	int width = 0;
	int height = 0;

	int channels = 0;
	void *data = stbi_load(filename, &width, &height, &channels, 0);
	if (!data) { return (Texture){0}; }

	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(
	    GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glBindTexture(GL_TEXTURE_2D, 0);
	stbi_image_free(data);

	return (Texture){
	    .id = id,
	    .width = width,
	    .height = height,
	};
}

SpriteBatch::SpriteBatch(int vertexCap)
    : m_shader("./resources/shaders/spriteBatch.vert",
               "./resources/shaders/spriteBatch.frag",
               true),
      m_texture(0)
{
	m_vertexCap = vertexCap;

	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glGenBuffers(1, &m_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertexCap, nullptr, GL_DYNAMIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
	    0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
	    1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, texcoord));

	memset(&m_matrix.cols, 0, 4 * 4 * sizeof(float));
}

void
SpriteBatch::flush()
{
	if (m_vertices.empty()) return;

	glUseProgram(m_shader.getProgram());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_texture);

	GLint textureLoc = glGetUniformLocation(m_shader.getProgram(), "u_texture");
	GLint mvpLoc = glGetUniformLocation(m_shader.getProgram(), "u_mvp");
	glUniform1i(textureLoc, 0);
	glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, m_matrix.cols[0]);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex) * m_vertices.size(), m_vertices.data());

	glBindVertexArray(m_vao);
	glDrawArrays(GL_TRIANGLES, 0, m_vertices.size());

	m_vertices.clear();
}

void
SpriteBatch::texture(GLuint id)
{
	if (m_texture != id)
	{
		flush();
		m_texture = id;
	}
}

void
SpriteBatch::mvp(Matrix mat)
{
	if (memcmp(&m_matrix.cols, &mat.cols, sizeof(Matrix)) == 0)
	{
		flush();
		m_matrix = mat;
	}
}

void
SpriteBatch::pushVertex(float x, float y, float u, float v)
{
	if (m_vertices.size() == m_vertexCap) flush();
	Vertex vertex = {{x, y}, {u, v}};
	m_vertices.emplace_back(vertex);
}

Camera::Camera() { resetView(); }

void
Camera::resetView()
{
	m_center = {0.0f, 0.0f};
	m_zoom = 1.0f;
}

glm::vec2
Camera::convertScreenToWorld(glm::vec2 ps) const
{
	auto w = float(m_width);
	auto h = float(m_height);
	float u = ps.x / w;
	float v = (h - ps.y) / h;

	float ratio = w / h;
	glm::vec2 extents = {m_zoom * ratio * m_viewHeight, m_zoom * m_viewHeight};

	glm::vec2 lower = m_center - extents;
	glm::vec2 upper = m_center + extents;

	glm::vec2 pw = {(1.0f - u) * lower.x + u * upper.x, (1.0f - v) * lower.y + v * upper.y};
	return pw;
}

glm::vec2
Camera::convertWorldToScreen(glm::vec2 pw) const
{
	auto w = float(m_width);
	auto h = float(m_height);
	float ratio = w / h;
	glm::vec2 extents = {m_zoom * ratio * m_viewHeight, m_zoom * m_viewHeight};

	glm::vec2 lower = m_center - extents;
	glm::vec2 upper = m_center + extents;

	float u = (pw.x - lower.x) / (upper.x - lower.x);
	float v = (pw.y - lower.y) / (upper.y - lower.y);

	glm::vec2 ps = {u * w, (1.0f - v) * h};
	return ps;
}

void
Camera::buildProjectionMatrix(float *m, float zBias) const
{
	float ratio = float(m_width) / float(m_height);
	glm::vec2 extents = {m_zoom * ratio * m_viewHeight, m_zoom * m_viewHeight};

	glm::vec2 lower = m_center - extents;
	glm::vec2 upper = m_center + extents;
	float w = upper.x - lower.x;
	float h = upper.y - lower.y;

	m[0] = 2.0f / w;
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;

	m[4] = 0.0f;
	m[5] = 2.0f / h;
	m[6] = 0.0f;
	m[7] = 0.0f;

	m[8] = 0.0f;
	m[9] = 0.0f;
	m[10] = -1.0f;
	m[11] = 0.0f;

	m[12] = -2.0f * m_center.x / w;
	m[13] = -2.0f * m_center.y / h;
	m[14] = zBias;
	m[15] = 1.0f;
}

void
Camera::setSize(int width, int height)
{
	m_width = width;
	m_height = height;
}

void
Camera::setCenter(float x, float y)
{
	m_center.x = x;
	m_center.y = y;
}

void
Camera::setZoom(float zoom)
{
	m_zoom = zoom;
}

int
Camera::getWidth() const
{
	return m_width;
}

int
Camera::getHeight() const
{
	return m_height;
}

float
Camera::getZoom() const
{
	return m_zoom;
}

const glm::vec2 &
Camera::getCenter() const
{
	return m_center;
}

void
Camera::translate(float x, float y)
{
	m_center.x -= x;
	m_center.y -= y;
}

void
Camera::scale(float s, float x, float y)
{
}

glm::vec4
Camera::getBoundingBox() const
{
	float ratio = float(m_width) / float(m_height);
	glm::vec2 extents{ratio * m_viewHeight * m_zoom, m_viewHeight * m_zoom};
	return {m_center - extents, m_center + extents};
}
