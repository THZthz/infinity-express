#ifndef CANDYBOX_SCENE_HPP__
#define CANDYBOX_SCENE_HPP__

#include <functional> // since c++11
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>

// nanovg
#ifndef NANOVG_GL3
#	define NANOVG_GL3 1
#endif
#include "candybox/vg/VG.hpp"
#include "candybox/vg/VG_gl.hpp"
#include "candybox/vg/VG_gl_utils.hpp"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <candybox/glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace candybox {
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
		glViewport(0, 0, m_frameWidth, m_frameHeight);
		glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		nvgBeginFrame(m_vg, (float)m_winWidth, (float)m_winHeight, m_devicePixelRatio);

		// ...

		nvgEndFrame(m_vg);

		nvgluBlitFramebuffer(m_framebuffer, prevFBO); // blit to prev FBO and rebind it
#endif
	}

	virtual void renderUI() {

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

	int getWinWidth() const { return m_winWidth; }
	int getWinHeight() const { return m_winHeight; }
	int getFrameWidth() const { return m_frameWidth; }
	int getFrameHeight() const { return m_frameHeight; }

	Camera &getCamera() { return m_camera; }

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
	static void charListener(GLFWwindow *window, unsigned int c);

	/*----------------------------------------------------------------------------*/
private:
	void mainLoop();


	/*----------------------------------------------------------------------------*/
protected:
	std::string m_name;

	bool m_showUI = true;

	Camera m_camera;

	float m_frameTime = 0.f;
	int32_t m_frame = 0;
	float m_prevTime = 0.f;
	float m_accumulator = 0.f;
	float m_hertz = 80.f; // game logic update rate

	bool m_preloaded = false;
	bool m_running = false;
	bool m_shouldRestart = false;
	bool m_shouldCleanup = true;

	NVGcontext *m_vg = nullptr;
	GLFWwindow *m_window;
	std::unique_ptr<Framebuffer> m_framebuffer;

	float m_windowScale = 1.f;
	float m_framebufferScale = 1.f;

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



} // namespace candybox

#endif // CANDYBOX_SCENE_HPP__
