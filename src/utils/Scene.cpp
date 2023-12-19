#include "utils/Scene.hpp"
#include <memory>
#include "GLFW/glfw3.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

using namespace ie;

Scene::Camera::Camera() { resetView(); }

void
Scene::Camera::resetView()
{
	m_center = {0.0f, 0.0f};
	m_zoom = 1.0f;
}

glm::vec2
Scene::Camera::convertScreenToWorld(glm::vec2 ps) const
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
Scene::Camera::convertWorldToScreen(glm::vec2 pw) const
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
Scene::Camera::buildProjectionMatrix(float *m, float zBias) const
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
Scene::Camera::setSize(int width, int height)
{
	m_width = width;
	m_height = height;
}

void
Scene::Camera::setCenter(float x, float y)
{
	m_center.x = x;
	m_center.y = y;
}

void
Scene::Camera::setZoom(float zoom)
{
	m_zoom = zoom;
}

int
Scene::Camera::getWidth() const
{
	return m_width;
}

int
Scene::Camera::getHeight() const
{
	return m_height;
}

float
Scene::Camera::getZoom() const
{
	return m_zoom;
}

const glm::vec2 &
Scene::Camera::getCenter() const
{
	return m_center;
}

void
Scene::Camera::translate(float x, float y)
{
	m_center.x -= x;
	m_center.y -= y;
}

void
Scene::Camera::scale(float s, float x, float y)
{
}

glm::vec4
Scene::Camera::getBoundingBox() const
{
	float ratio = float(m_width) / float(m_height);
	glm::vec2 extents{ratio * m_viewHeight * m_zoom, m_viewHeight * m_zoom};
	return {m_center - extents, m_center + extents};
}

Scene::Scene(const std::string &name, int width, int height)
    : m_name(name), m_winWidth(width), m_winHeight(height)

{
	// initialize glfw library globally, this is safe to call multiple times
	if (!glfwInit()) throw std::runtime_error("Failed to initialize GLFW");

	glfwSetErrorCallback(errorListener);

	// create a window
//#define IE_SCENE_IMPL_OPENGL_ES2
#if defined(IE_SCENE_IMPL_OPENGL_ES2) // Decide GL+GLSL versions  // TODO
	// GL ES 2.0 + GLSL 100
	const char *glsl_version = "#version 100";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
	// GL 3.2 + GLSL 150
	const char *glsl_version = "#version 150";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
#else
	// GL 3.0 + GLSL 130
	const char *glsl_version = "#version 130";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
//	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
	glfwWindowHint(GLFW_SAMPLES, 4);
	m_window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
	if (!m_window)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create window");
	}

	if (GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor())
	{
#ifdef __APPLE__
		glfwGetMonitorContentScale(primaryMonitor, &m_framebufferScale, &m_framebufferScale);
#else
		glfwGetMonitorContentScale(primaryMonitor, &m_windowScale, &m_windowScale);
#endif
	}

#ifdef __APPLE__
	glfwGetWindowContentScale(g_mainWindow, &m_framebufferScale, &m_framebufferScale);
#else
	glfwGetWindowContentScale(m_window, &m_windowScale, &m_windowScale);
#endif

	// make gl context active for the window
	glfwMakeContextCurrent(m_window);

	// load opengl functions from graphics cards
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		throw std::runtime_error("Failed to load address of OpenGL functions");

	// create nanovg context
	m_vg = nvglCreate(/*NVG_SRGB | */ NVG_DEBUG);
	if (!m_vg) throw std::runtime_error("Failed to create nanovg context");

	// create framebuffer for blitting
	m_framebuffer.reset(new Framebuffer(m_vg, 0, 0, Framebuffer::NO_NVG_IMAGE /* | NVG_SRGB*/));

	// add callbacks to the window
	glfwSetWindowUserPointer(m_window, this);
	glfwSetCursorPosCallback(m_window, cursorPosListener);
	glfwSetKeyCallback(m_window, keyListener);
	glfwSetMouseButtonCallback(m_window, mouseButtonListener);
	glfwSetFramebufferSizeCallback(m_window, framebufferSizeListener);
	glfwSetScrollCallback(m_window, scrollListener);
	glfwSetCharCallback(m_window, charListener);

	// initialize imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	// Setup Platform/Renderer backends
	if (!ImGui_ImplGlfw_InitForOpenGL(m_window, false))
		throw std::runtime_error("ImGui_ImplGlfw_InitForOpenGL failed");
	if (!ImGui_ImplOpenGL3_Init(nullptr))
		throw std::runtime_error("ImGui_ImplOpenGL3_Init failed");

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	// - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != nullptr);
}

Scene::~Scene()
{
	// destroy imgui context
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();


	assert(!m_preloaded);
	nvglDelete(m_vg);
	glfwDestroyWindow(m_window);
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
	// NOTE: in game logic we should not use glfwGetTime, but use std::chrono instead.
	m_running = true;
	m_frameTime = 0.f;
	m_prevTime = 0.f;
	m_accumulator = 0.f;
	m_frame = 0;
	glfwSwapInterval(0);
	glfwSetTime(0);
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
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
	if (ImGui::GetIO().WantCaptureMouse) return;
	auto *ctx = static_cast<Scene *>(glfwGetWindowUserPointer(window));
	ctx->onMouseButton(button, action, mods);
	for (const auto &cb : ctx->m_mouseButtonCallbacks) cb(window, button, action, mods);
}

void
Scene::cursorPosListener(GLFWwindow *window, double xpos, double ypos)
{
	ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos); // TODO divide window scale ?
	auto *ctx = static_cast<Scene *>(glfwGetWindowUserPointer(window));
	ctx->onCursorPos(xpos, ypos);
	for (const auto &cb : ctx->m_cursorPosCallbacks) cb(window, xpos, ypos);
}

void
Scene::scrollListener(GLFWwindow *window, double xoffset, double yoffset)
{
	ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
	if (ImGui::GetIO().WantCaptureMouse) return;
	auto *ctx = static_cast<Scene *>(glfwGetWindowUserPointer(window));
	ctx->onScroll(xoffset, yoffset);
	for (const auto &cb : ctx->m_scrollCallbacks) cb(window, xoffset, yoffset);
}

void
Scene::keyListener(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
	if (ImGui::GetIO().WantCaptureKeyboard) return;
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
Scene::charListener(GLFWwindow *window, unsigned int c)
{
	ImGui_ImplGlfw_CharCallback(window, c);
}

void
Scene::mainLoop()
{
	double time1 = glfwGetTime(); // start time of current frame

	glfwGetWindowSize(m_window, &m_winWidth, &m_winHeight);
	glfwGetFramebufferSize(m_window, &m_frameWidth, &m_frameHeight);

	// Calculate pixel ration for hi-dpi devices.
	m_devicePixelRatio = (float)m_frameWidth / (float)m_winWidth;

	update(m_frameTime);
	render();

	if (m_showUI)
	{
		double mx = 0, my = 0;
		glfwGetCursorPos(m_window, &mx, &my);
		ImGui_ImplGlfw_CursorPosCallback(m_window, mx / m_windowScale, my / m_windowScale);
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui_ImplGlfw_CursorPosCallback(m_window, mx / m_windowScale, my / m_windowScale);

		const float cameraWidth = float(m_winWidth) / m_windowScale;
		const float cameraHeight = float(m_winHeight) / m_windowScale;
		ImGuiIO &io = ImGui::GetIO();
		io.DisplaySize.x = cameraWidth;
		io.DisplaySize.y = cameraHeight;
		io.DisplayFramebufferScale.x = float(m_frameWidth) / cameraWidth;
		io.DisplayFramebufferScale.y = float(m_frameHeight) / cameraHeight;

		renderUI();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	glfwSwapBuffers(m_window);
	glfwPollEvents();

	// Limit frame rate
	double time2 = glfwGetTime();
	//	double targetTime = time1 + 1.0f / m_hertz;
	//	int loopCount = 0;
	//	while (time2 < targetTime)
	//	{
	//		time2 = glfwGetTime();
	//		++loopCount;
	//	}
	m_frameTime = (float)(time2 - time1);
	++m_frame;
}
