#include "utils/Scene.hpp"

using namespace ie;

Scene::Scene(const std::string &name, int width, int height)
    : m_name(name), m_winWidth(width), m_winHeight(height)

{
	// set transform matrix to identity
	nvgTransformIdentity(m_transform);
	nvgTransformIdentity(m_inverseTransform);


	// initialize glfw library globally, this is safe to call multiple times
	if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW");

	glfwSetErrorCallback(errorListener);

	// create a window
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
	glfwWindowHint(GLFW_SAMPLES, 4);
	m_window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
	if (!m_window)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create window");
	}

	// make gl context active for the window
	glfwMakeContextCurrent(m_window);

	// load opengl functions from graphics cards
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		throw std::runtime_error("Failed to load address of OpenGL functions");

	// create nanovg context
	m_vg = nvglCreate(NVG_SRGB | NVG_DEBUG);
	if (!m_vg) throw std::runtime_error("Failed to create nanovg context");

	// create framebuffer for blitting
	m_framebuffer = nvgluCreateFramebuffer(m_vg, 0, 0, NVGLU_NO_NVG_IMAGE | NVG_SRGB);

	// add callbacks to the window
	glfwSetWindowUserPointer(m_window, this);
	glfwSetCursorPosCallback(m_window, cursorPosListener);
	glfwSetKeyCallback(m_window, keyListener);
	glfwSetMouseButtonCallback(m_window, mouseButtonListener);
	glfwSetFramebufferSizeCallback(m_window, framebufferSizeListener);
	glfwSetScrollCallback(m_window, scrollListener);
}

Scene::~Scene()
{
	nvgluDeleteFramebuffer(m_framebuffer);
	nvglDelete(m_vg);
	glfwTerminate();
}

void
Scene::start()
{
start:
	if (!m_preloaded)
	{
		preload();
		m_preloaded = true;
	}
	m_running = true;
	m_prevTime = 0.f;
	m_accumulator = 0.f;
	glfwSwapInterval(0);
	glfwSetTime(0); // TODO
	while (m_running && !glfwWindowShouldClose(m_window)) { mainLoop(); }
	if (m_preloaded && m_shouldCleanup)
	{
		cleanup();
		m_preloaded = false;
	}
	if (m_shouldRestart)
	{
		m_shouldRestart = false;
		goto start;
	}
}

void
Scene::stop()
{
	m_running = false;
}

void
Scene::restart()
{
	m_running = false;
	m_shouldRestart = true;
}

void
Scene::pause()
{
	m_running = false;
	m_shouldRestart = false;
	m_shouldCleanup = false;
}

void
Scene::resume()
{
	m_running = true;
	m_shouldCleanup = true;
}

template <typename T>
void
Scene::addListener(std::vector<T> &callbackList, T callback)
{
	callback.emplace_back(callback);
}

template <typename T>
void
Scene::removeListener(std::vector<T> &callbackList, const T &callback)
{
	typename std::vector<T>::size_type index = 0;
	for (; index < callbackList.size(); index++)
	{
		// if the callback appeared multiple times in the list, we remove them all
		if (callbackList[index] == callback)
		{
			callbackList.erase(callbackList.begin() + index);
			index--;
		}
	}
}

void
ie::Scene::errorListener(int code, const char *desc)
{
	printf("[GLFW] [ERROR] %d: %s\n", code, desc);
}

void
Scene::mouseButtonListener(GLFWwindow *window, int button, int action, int mods)
{
	auto *ctx = static_cast<Scene *>(glfwGetWindowUserPointer(window));
	ctx->onMouseButton(button, action, mods);
	for (const auto &cb : ctx->m_mouseButtonCallbacks) cb(window, button, action, mods);
}

void
Scene::cursorPosListener(GLFWwindow *window, double xpos, double ypos)
{
	auto *ctx = static_cast<Scene *>(glfwGetWindowUserPointer(window));
	ctx->onCursorPos(xpos, ypos);
	for (const auto &cb : ctx->m_cursorPosCallbacks) cb(window, xpos, ypos);
}

void
Scene::scrollListener(GLFWwindow *window, double xoffset, double yoffset)
{
	auto *ctx = static_cast<Scene *>(glfwGetWindowUserPointer(window));
	ctx->onScroll(xoffset, yoffset);
	for (const auto &cb : ctx->m_scrollCallbacks) cb(window, xoffset, yoffset);
}

void
Scene::keyListener(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	auto *ctx = static_cast<Scene *>(glfwGetWindowUserPointer(window));
	ctx->onKey(key, scancode, action, mods);
	for (const auto &cb : ctx->m_keyCallbacks) cb(window, key, scancode, action, mods);
}

void
Scene::framebufferSizeListener(GLFWwindow *window, int width, int height)
{
	auto *ctx = static_cast<Scene *>(glfwGetWindowUserPointer(window));
	ctx->onFramebufferSize(width, height);
	for (const auto &cb : ctx->m_framebufferSizeCallbacks) cb(window, width, height);
}

void
Scene::mainLoop()
{
	glfwGetWindowSize(m_window, &m_winWidth, &m_winHeight);
	glfwGetFramebufferSize(m_window, &m_frameWidth, &m_frameHeight);

	// Calculate pixel ration for hi-dpi devices.
	m_devicePixelRatio = (float)m_frameWidth / (float)m_winWidth;

	// calculate current transform // TODO
	float t[6];
	nvgTransformIdentity(m_transform);
	nvgTransformTranslate(t, m_translation.x, m_translation.y);
	nvgTransformPremultiply(m_transform, t);
	nvgTransformScale(t, m_scale, m_scale);
	nvgTransformPremultiply(m_transform, t);
	nvgTransformRotate(t, m_rotation);
	nvgTransformPremultiply(m_transform, t);

	// get the position of the mouse in world space
	float pwx, pwy;
	nvgTransformInverse(m_inverseTransform, m_transform);
	double x, y;
	glfwGetCursorPos(m_window, &x, &y);
	nvgTransformPoint(&pwx, &pwy, m_inverseTransform, (float)x, (float)y);
	m_pointer.x = (float)x;
	m_pointer.y = (float)y;
	m_pointerWorld.x = pwx;
	m_pointerWorld.y = pwy;

	// do not allow large time gap and improve stability
	double fixedDelta = 1.f / m_hertz;
	double curTime = glfwGetTime(); // nvgTime_sec(nvgTimeNow());
	double deltaTime = curTime - m_prevTime;
	//		if (deltaTime > 0.2f) deltaTime = 0.2f; // TODO
	m_accumulator += deltaTime;
	m_delta = (float)deltaTime;
	while (m_accumulator > fixedDelta)
	{
		update((float)fixedDelta); // TODO
		m_accumulator -= (float)fixedDelta;
	}

	render();
	m_prevTime = curTime;

	glfwSwapBuffers(m_window);
	glfwPollEvents();
}
