#ifndef IE_SCENE_HPP
#define IE_SCENE_HPP

#include <functional> // since c++11
#include <string>
#include <stdexcept>
#include <vector>

// nanovg
#ifndef NANOVG_GL3
#	define NANOVG_GL3 1
#endif
#include "nanovg.h"
#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

#include <glm/vec2.hpp>
#include "glad/glad.h"
#include <GLFW/glfw3.h>

namespace ie {
class Scene
{
public:
	Scene(const std::string &name, int width, int height);
	~Scene();

	/*----------------------------------------------------------------------------*/
public:
	void start();
	void stop();
	void restart();
	void pause();
	void resume();

	/*----------------------------------------------------------------------------*/

public:
	virtual void preload() { }
	virtual void cleanup() { }
	virtual void update(float delta) { }

	virtual void render()
	{
#if 0
        // a template:

		int prevFBO = nvgluBindFramebuffer(m_framebuffer);
		nvgluSetFramebufferSize(m_framebuffer, m_frameWidth, m_frameHeight, 0);

		// Update and render
		nvgluSetViewport(0, 0, m_frameWidth, m_frameHeight);
		nvgluClear(nvgRGBAf(0.3f, 0.3f, 0.32f, 1.0f));

		nvgBeginFrame(m_vg, (float)m_winWidth, (float)m_winHeight, m_devicePixelRatio);

		// ...

		nvgEndFrame(m_vg);

		nvgluBlitFramebuffer(m_framebuffer, prevFBO); // blit to prev FBO and rebind it
#endif
	}

	/*----------------------------------------------------------------------------*/

public:
	virtual void onKey(int key, int scancode, int action, int mods) { }
	virtual void onFramebufferSize(int width, int height) { }
	virtual void onScroll(double offsetX, double offsetY) { }
	virtual void onCursorPos(double x, double y) { }
	virtual void onMouseButton(int button, int action, int mods) { }

	/*----------------------------------------------------------------------------*/

public:
	bool isRunning() const { return m_running; }
	const std::string &getName() const { return m_name; }
	NVGcontext *getVg() const { return m_vg; }
	float getFreq() const { return m_hertz; }
	void setFreq(float freq) { m_hertz = freq; }
	const glm::vec2 &getTranslation() const { return m_translation; }
	float getScale() const { return m_scale; }
	float getRotation() const { return m_rotation; }
	const float *getTransform() const { return m_transform; }
	const float *getInverseTransform() const { return m_inverseTransform; }

	void setTranslation(const glm::vec2 &trans)
	{
		m_shouldUpdateInvTransform = true;
		m_translation = trans;
	}

	void setScale(float scale)
	{
		m_shouldUpdateInvTransform = true;
		m_scale = scale;
	}

	void setTransX(float x)
	{
		m_shouldUpdateInvTransform = true;
		m_translation.x = x;
	}

	void setTransY(float y)
	{
		m_shouldUpdateInvTransform = true;
		m_translation.y = y;
	}

	void translate(float x, float y)
	{
		m_shouldUpdateInvTransform = true;
		m_translation.x += x;
		m_translation.y += y;
	}

	void rotate(float rad)
	{
		m_shouldUpdateInvTransform = true;
		m_rotation += rad;
	}

	/*----------------------------------------------------------------------------*/

	glm::vec2 getPointer() const { return m_pointer; }
	const glm::vec2 &getPointerWorld() const { return m_pointerWorld; }

	/*----------------------------------------------------------------------------*/
private:
	template <typename T>
	static void addListener(std::vector<T> &callbackList, T callback);

	template <typename T>
	static void removeListener(std::vector<T> &callbackList, const T &callback);

	static void errorListener(int code, const char *desc);
	static void mouseButtonListener(GLFWwindow *window, int button, int action, int mods);
	static void cursorPosListener(GLFWwindow *window, double xpos, double ypos);
	static void scrollListener(GLFWwindow *window, double xoffset, double yoffset);
	static void keyListener(GLFWwindow *window, int key, int scancode, int action, int mods);
	static void framebufferSizeListener(GLFWwindow *window, int width, int height);

	/*----------------------------------------------------------------------------*/
private:
	void mainLoop();


	/*----------------------------------------------------------------------------*/
protected:
	std::string m_name;

	glm::vec2 m_translation{0.f, 0.f};
	float m_scale = 1.f;
	float m_rotation{0.f}; // in radians
	float m_transform[6];
	float m_inverseTransform[6];
	bool m_shouldUpdateInvTransform{false};

	glm::vec2 m_pointer{};
	glm::vec2 m_pointerWorld{};

	double m_accumulator{};
	double m_prevTime{};
	float m_fps{};
	float m_hertz{60}; // game logic update rate.
	float m_delta{};

	bool m_preloaded = false;
	bool m_running = false;
	bool m_shouldRestart = false;
	bool m_shouldCleanup = true;

	NVGcontext *m_vg = nullptr;
	GLFWwindow *m_window;
	NVGLUframebuffer *m_framebuffer = nullptr;

	int m_winWidth;
	int m_winHeight;
	int m_frameWidth{};
	int m_frameHeight{};
	float m_devicePixelRatio{};

	using KeyCallback =
	    std::function<bool(GLFWwindow *window, int key, int scancode, int action, int mods)>;
	using FramebufferSizeCallback =
	    std::function<bool(GLFWwindow *window, int width, int height)>;
	using CursorPosCallback = std::function<bool(GLFWwindow *window, double x, double y)>;
	using MouseButtonCallback =
	    std::function<bool(GLFWwindow *window, int button, int action, int mods)>;
	using ScrollCallback =
	    std::function<bool(GLFWwindow *window, double offsetX, double offsetY)>;
	std::vector<KeyCallback> m_keyCallbacks;
	std::vector<FramebufferSizeCallback> m_framebufferSizeCallbacks;
	std::vector<CursorPosCallback> m_cursorPosCallbacks;
	std::vector<MouseButtonCallback> m_mouseButtonCallbacks;
	std::vector<ScrollCallback> m_scrollCallbacks;
};
} // namespace ie

#endif // IE_SCENE_HPP
