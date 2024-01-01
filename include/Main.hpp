#ifndef CANDYBOX_MAIN_SCENE_HPP__
#define CANDYBOX_MAIN_SCENE_HPP__

#include <memory>
#include "World.hpp"

#define SHADER_TEXT(x) "#version 330 core\n" #x
//#define SHADER_TEXT(x) "#version 100\n" #x

class MainWorld : public PhysicsWorld
{
public:
	explicit MainWorld(candybox::Scene *scene) : PhysicsWorld(scene)
	{
		printf("Infinity Express v%s\n", INFINITY_EXPRESS_VERSION_STR);
		printf("\tc++ standard version: %ld\n", __cplusplus);
		printf("\tOpenGL %s, GLSL %s\n", glGetString(GL_VERSION),
		       glGetString(GL_SHADING_LANGUAGE_VERSION));
	}

	void initialize() override
	{
		PhysicsWorld::initialize();
		load();
	}

	void destroy() override { PhysicsWorld::destroy(); }

	void load();

private:
	b2BodyId m_attachmentId = b2_nullBodyId;
	b2BodyId m_platformId = b2_nullBodyId;
	float m_speed = 0;
};

class ShaderCRT : public candybox::Shader
{
public:
	ShaderCRT();

	~ShaderCRT()
	{
		candybox::Shader::~Shader();
		glDeleteVertexArrays(1, &m_vertexArray);
		glDeleteBuffers(1, &m_vertexBuffer);
		glDeleteBuffers(1, &m_elementBuffer);
	}

	void use(float frameWidth, float frameHeight) const;

private:
	GLint m_timeLoc = 0, m_resolutionLoc = 0;
	unsigned int m_vertexBuffer = 0, m_vertexArray = 0, m_elementBuffer = 0;
};

class ShaderLight : public candybox::Shader
{
public:
	ShaderLight();
	~ShaderLight();

	GLint getMatrixLoc() const { return m_matrixLoc; }
	GLint getColorLoc() const { return m_colorLoc; }

	void use();

private:
	GLuint m_vertexBuffer = 0;
	GLint m_matrixLoc = 0, m_colorLoc = 0;
};

class ShaderShadow : public candybox::Shader
{
public:
	ShaderShadow();
	~ShaderShadow();

	GLint getMatrixLoc() const { return m_matrixLoc; }
	GLint getLightLoc() const { return m_lightLoc; }

	void use();

private:
	GLuint m_vertexBuffer = 0;
	GLint m_matrixLoc = 0, m_lightLoc = 0;
};


class App : public candybox::Scene
{
public:
	App() : candybox::Scene("Demo", 800, 800), m_physicsWorld(this)
	{
		m_camera.setSize(m_winWidth, m_winHeight);
	}

	void preload() override { m_physicsWorld.initialize(); }

	void cleanup() override { m_physicsWorld.destroy(); }

	void update(float delta) override { m_physicsWorld.update(delta); }

	void render() override;

	void renderUI() override;

	// ----------------------------------------------------------------
public:
	void onKey(int key, int scancode, int action, int mods) override { }

	void onMouseButton(int button, int action, int mods) override
	{
		double xd, yd;
		glfwGetCursorPos(m_window, &xd, &yd);
		glm::vec2 pw = m_camera.convertScreenToWorld(
		    {float(xd) / m_windowScale, float(yd) / m_windowScale});

		m_physicsWorld.onMouseButton({pw.x, pw.y}, button, action, mods);

		if (button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			if (action == GLFW_PRESS)
			{
				m_isRightMousePressed = true;
				m_prevPos = pw;
			}
			else { m_isRightMousePressed = false; }
		}
	}

	void onCursorPos(double x, double y) override
	{
		glm::vec2 pw = m_camera.convertScreenToWorld({x / m_windowScale, y / m_windowScale});
		m_physicsWorld.onCursorPos({pw.x, pw.y});

		if (m_isRightMousePressed)
		{
			glm::vec2 d = pw - m_prevPos;
			m_camera.translate(d.x, d.y);
			m_prevPos = m_camera.convertScreenToWorld({x, y});
		}
	}

	void onScroll(double x, double y) override
	{
		if (y > 0) m_camera.setZoom(m_camera.getZoom() * 0.9f);
		else if (y < 0) m_camera.setZoom(m_camera.getZoom() * 1.1f);
	}

	void onFramebufferSize(int w, int h) override
	{
		m_camera.setSize(
		    static_cast<int>((float)w / m_windowScale),
		    static_cast<int>((float)h / m_windowScale));
	}

private:
	void drawLights();

private:
	bool m_isRightMousePressed = false;
	glm::vec2 m_prevPos{0, 0};
	
	ShaderCRT m_shaderCRT;
	ShaderLight m_shaderLight;
	ShaderShadow m_shaderShadow;

	MainWorld m_physicsWorld;

	// graphics control
	bool m_enableCRT = false;
};

#endif // CANDYBOX_MAIN_SCENE_HPP__
