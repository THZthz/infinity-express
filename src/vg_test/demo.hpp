#ifndef DEMO_H
#define DEMO_H

#include "platform.hpp"
#include "candybox/vg/VG.hpp"
#include "perf.hpp"
#include "candybox/vg/VG_sw_utils.hpp"
#include "candybox/Scene.hpp"

#ifdef __cplusplus
extern "C" {
#endif

struct DemoData
{
	int fontNormal, fontBold, fontIcons, fontEmoji;
	int images[12];
};
typedef struct DemoData DemoData;

int loadDemoData(NVGcontext* vg, DemoData* data, int imgflags);
void freeDemoData(NVGcontext* vg, DemoData* data);
void renderDemo(
    NVGcontext* vg,
    float mx,
    float my,
    float width,
    float height,
    float t,
    int blowup,
    DemoData* data);

void saveScreenShot(int w, int h, int premult, const char* name);

#ifdef __cplusplus
}
#endif


class VgApp : public candybox::Scene
{
public:
	VgApp() : candybox::Scene("nanovg", 800, 600)
	{
		//#ifdef DEMO_MSAA // TODO
		//	glfwWindowHint(GLFW_SAMPLES, 4);
		//#endif
		m_camera.setSize(m_winWidth, m_winHeight);
	}

	void preload() override;

	void cleanup() override;

	void update(float delta) override { }

	void render() override;

	// ----------------------------------------------------------------
public:
	void onKey(int key, int scancode, int action, int mods) override;

	void onMouseButton(int button, int action, int mods) override { }

	void onCursorPos(double x, double y) override { }

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
	PerfGraph m_fps, m_cpuGraph, m_gpuGraph;
	GPUtimer m_gpuTimer;
	double prevt = 0, cpuTime = 0;

	void* m_blitterFB = nullptr;
	NVGSWUblitter* m_blitter = nullptr;

	DemoData m_data;
};


#endif // DEMO_H
