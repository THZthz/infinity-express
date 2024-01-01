#ifndef NANOVG_GL_UTILS_H
#define NANOVG_GL_UTILS_H

#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <candybox/glad/glad.h>
#include "candybox/vector.hpp"
#include "VG.hpp"

namespace candybox {

class Camera
{
public:
	Camera();

	void resetView();
	glm::vec2 convertScreenToWorld(glm::vec2 screenPoint) const;
	glm::vec2 convertWorldToScreen(glm::vec2 worldPoint) const;

	glm::vec4 getBoundingBox() const;

	/// Convert from world coordinates to normalized device coordinates.
	/// http://www.songho.ca/opengl/gl_projectionmatrix.html
	/// This also includes the view transform
	/// \param m 4x4 projection matrix
	void buildProjectionMatrix(float* m, float zBias = 1) const;

	void setSize(int width, int height);
	void setCenter(float x, float y);
	void setZoom(float zoom);

	int getWidth() const;
	int getHeight() const;
	float getZoom() const;
	const glm::vec2& getCenter() const;

	void translate(float x, float y);
	void scale(float s, float x, float y);

private:
	glm::vec2 m_center{};
	float m_zoom = 1.f;
	int32_t m_width = 0; // window's width
	int32_t m_height = 0; // window's height

	float m_viewHeight = 20.f;
};



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
	static constexpr char* vertex = (char* const)"";
	static constexpr char* fragment = (char* const)"";

	Shader m_shader{vertex, fragment};
};

class SpriteBatch
{
public:
	struct Texture
	{
		GLuint id;
		int width;
		int height;
	};

	struct Matrix
	{
		float cols[4][4];
	};

	struct Vertex
	{
		float position[2];
		float texcoord[2];
	};

private:
	Shader m_shader;

	// vertex buffer data
	GLuint m_vao = 0;
	GLuint m_vbo = 0;
	std::vector<Vertex> m_vertices;
	int m_vertexCap;

	// uniform values
	GLuint m_texture;
	Matrix m_matrix;

public:
	SpriteBatch(int vertexCap);

	void flush();
	void texture(GLuint id);
	void mvp(Matrix mat);
	void pushVertex(float x, float y, float u, float v);

	Texture createTexture(const char* filename);
};

} // namespace candybox

#endif // NANOVG_GL_UTILS_H

#ifdef NANOVG_GLU_IMPLEMENTATION




#undef NANOVG_GLU_IMPLEMENTATION
#endif // NANOVG_GLU_IMPLEMENTATION
