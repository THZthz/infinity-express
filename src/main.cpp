#include <algorithm>
#include <cassert>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <cstdint>
#include <cstdlib>

#include "utils/Scene.hpp"
#include "utils/Color.hpp"
#include "utils/TaskScheduler.hpp"
#include "utils/Linear.hpp"
#include "utils/vector.hpp"
#include "Main.hpp"
#include "CellAutomata.hpp"
#include "./vg_test/demo.hpp"

// implements nanovg
#include "utils/VG.hpp"
//#define NANOVG_GLES2 1
#define NANOVG_GL3 1
#define NANOVG_GL_IMPLEMENTATION
#include "utils/VG_gl.hpp"
#define NANOVG_GLU_IMPLEMENTATION
#include "utils/VG_gl_utils.hpp"
//#define NVGSWU_GLES2
#define NVGSWU_GL3
#define NANOVG_SW_IMPLEMENTATION
#include "utils/VG_sw.hpp"
#include "utils/VG_sw_utils.hpp"

#include "utils/ref_ptr.hpp"

int
main()
{
	//			VgApp app;
//	App app;
//	app.start();

	//	CellAutomata gen;
	//	gen.generate();

	return EXIT_SUCCESS;
}



void
MainWorld::load()
{
	b2BodyId groundId;
	{
		b2BodyDef bodyDef = b2DefaultBodyDef();
		groundId = b2World_CreateBody(m_worldId, &bodyDef);

		b2Segment segment = {{-20.0f, 0.0f}, {20.0f, 0.0f}};
		b2ShapeDef shapeDef = b2DefaultShapeDef();
		b2Body_CreateSegment(groundId, &shapeDef, &segment);
	}

	// Define attachment
	{
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = b2_dynamicBody;
		bodyDef.position = {0.0f, 3.0f};
		m_attachmentId = b2World_CreateBody(m_worldId, &bodyDef);

		b2Polygon box = b2MakeBox(0.5f, 2.0f);
		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.density = 1.0f;
		b2Body_CreatePolygon(m_attachmentId, &shapeDef, &box);
	}

	// Define platform
	{
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = b2_dynamicBody;
		bodyDef.position = {-4.0f, 5.0f};
		m_platformId = b2World_CreateBody(m_worldId, &bodyDef);

		b2Polygon box = b2MakeOffsetBox(0.5f, 4.0f, {4.0f, 0.0f}, 0.5f * b2_pi);

		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.friction = 0.6f;
		shapeDef.density = 2.0f;
		b2Body_CreatePolygon(m_platformId, &shapeDef, &box);

		b2RevoluteJointDef revoluteDef = b2DefaultRevoluteJointDef();
		b2Vec2 pivot = {0.0f, 5.0f};
		revoluteDef.bodyIdA = m_attachmentId;
		revoluteDef.bodyIdB = m_platformId;
		revoluteDef.localAnchorA = b2Body_GetLocalPoint(m_attachmentId, pivot);
		revoluteDef.localAnchorB = b2Body_GetLocalPoint(m_platformId, pivot);
		revoluteDef.maxMotorTorque = 50.0f;
		revoluteDef.enableMotor = true;
		b2World_CreateRevoluteJoint(m_worldId, &revoluteDef);

		b2PrismaticJointDef prismaticDef = b2DefaultPrismaticJointDef();
		b2Vec2 anchor = {0.0f, 5.0f};
		prismaticDef.bodyIdA = groundId;
		prismaticDef.bodyIdB = m_platformId;
		prismaticDef.localAnchorA = b2Body_GetLocalPoint(groundId, anchor);
		prismaticDef.localAnchorB = b2Body_GetLocalPoint(m_platformId, anchor);
		prismaticDef.localAxisA = {1.0f, 0.0f};
		prismaticDef.maxMotorForce = 1000.0f;
		prismaticDef.motorSpeed = 0.0f;
		prismaticDef.enableMotor = true;
		prismaticDef.lowerTranslation = -10.0f;
		prismaticDef.upperTranslation = 10.0f;
		prismaticDef.enableLimit = true;

		b2World_CreatePrismaticJoint(m_worldId, &prismaticDef);

		m_speed = 3.0f;
	}

	// Create a payload
	{
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = b2_dynamicBody;
		bodyDef.position = {0.0f, 8.0f};
		b2BodyId bodyId = b2World_CreateBody(m_worldId, &bodyDef);

		b2Polygon box = b2MakeBox(0.75f, 0.75f);

		b2ShapeDef shapeDef = b2DefaultShapeDef();
		shapeDef.friction = 0.6f;
		shapeDef.density = 2.0f;

		b2Body_CreatePolygon(bodyId, &shapeDef, &box);
	}
}


static void
transformMat4x4(float *mat, float x, float y, float rotate, float scale)
{
	const float c = scale * std::cos(rotate);
	const float s = scale * std::sin(rotate);

	mat[0] = c;
	mat[1] = -s;
	mat[2] = 0.f;
	mat[3] = 0.f;

	mat[4] = s;
	mat[5] = c;
	mat[6] = 0.f;
	mat[7] = 0.f;

	mat[8] = 0.f;
	mat[9] = 0.f;
	mat[10] = 1.f;
	mat[11] = 0.f;

	mat[12] = x;
	mat[13] = y;
	mat[14] = 0.f;
	mat[15] = 1.f;
}


// Quick vertex buffer to draw a light with.
// Just use a sprite in a real project, it makes the shape/gradient more flexible.
static const float g_lightSpriteVerts[] = {
    10,  10,  10,  10, //
    -10, 10,  -10, 10, //
    10,  -10, 10,  -10, //
    -10, -10, -10, -10, //
};

// Vertex format is {{a.x, a.y}, {b.x, b.y}, {s.x, s.y}} where:
// 'a' is the first endpoint of a shadow casting segment.
// 'b' is the seconnd endpoint
// 's' is the shadow coordinate, and selects which corner
// of the shadow quad this vertex corresponds to.
// This makes for a fair amount of redundant vertex data.
// Instancing will simplify packing the shadow data, but might be slower.
// NOTE: I'm using non-indexed geometry here to avoid adding index
//      buffer code to an otherwise fairly minimal code example.
//      This is NOT at all ideal, and you should really prefer
//      indexed triangles or instancing in your own code.
static const float minx1 = 0.f, miny1 = 0.f, maxx2 = 0.2f, maxy2 = 0.1f;
static const float minx3 = -0.4f, miny3 = -0.3f, maxx4 = -0.2f, maxy4 = -0.1f;
static const float g_shadowVerts[] = {
    minx1, miny1, maxx2, miny1, 0.0, 0.0, // Vertex A
    minx1, miny1, maxx2, miny1, 0.0, 1.0, // Vertex B
    minx1, miny1, maxx2, miny1, 1.0, 1.0, // Vertex C
    minx1, miny1, maxx2, miny1, 1.0, 1.0, // Vertex C
    minx1, miny1, maxx2, miny1, 1.0, 0.0, // Vertex D
    minx1, miny1, maxx2, miny1, 0.0, 0.0, // Vertex A

    maxx2, miny1, maxx2, maxy2, 0.0, 0.0, //
    maxx2, miny1, maxx2, maxy2, 0.0, 1.0, //
    maxx2, miny1, maxx2, maxy2, 1.0, 1.0, //
    maxx2, miny1, maxx2, maxy2, 1.0, 1.0, //
    maxx2, miny1, maxx2, maxy2, 1.0, 0.0, //
    maxx2, miny1, maxx2, maxy2, 0.0, 0.0, //

    maxx2, maxy2, minx1, maxy2, 0.0, 0.0, //
    maxx2, maxy2, minx1, maxy2, 0.0, 1.0, //
    maxx2, maxy2, minx1, maxy2, 1.0, 1.0, //
    maxx2, maxy2, minx1, maxy2, 1.0, 1.0, //
    maxx2, maxy2, minx1, maxy2, 1.0, 0.0, //
    maxx2, maxy2, minx1, maxy2, 0.0, 0.0, //

    minx1, maxy2, minx1, miny1, 0.0, 0.0, //
    minx1, maxy2, minx1, miny1, 0.0, 1.0, //
    minx1, maxy2, minx1, miny1, 1.0, 1.0, //
    minx1, maxy2, minx1, miny1, 1.0, 1.0, //
    minx1, maxy2, minx1, miny1, 1.0, 0.0, //
    minx1, maxy2, minx1, miny1, 0.0, 0.0, //

    minx3, miny3, maxx4, miny3, 0.0, 0.0, //  A
    minx3, miny3, maxx4, miny3, 0.0, 1.0, //  B
    minx3, miny3, maxx4, miny3, 1.0, 1.0, //  C
    minx3, miny3, maxx4, miny3, 1.0, 1.0, //  C
    minx3, miny3, maxx4, miny3, 1.0, 0.0, //  D
    minx3, miny3, maxx4, miny3, 0.0, 0.0, //  A

    maxx4, miny3, maxx4, maxy4, 0.0, 0.0, //
    maxx4, miny3, maxx4, maxy4, 0.0, 1.0, //
    maxx4, miny3, maxx4, maxy4, 1.0, 1.0, //
    maxx4, miny3, maxx4, maxy4, 1.0, 1.0, //
    maxx4, miny3, maxx4, maxy4, 1.0, 0.0, //
    maxx4, miny3, maxx4, maxy4, 0.0, 0.0, //

    maxx4, maxy4, minx3, maxy4, 0.0, 0.0, //
    maxx4, maxy4, minx3, maxy4, 0.0, 1.0, //
    maxx4, maxy4, minx3, maxy4, 1.0, 1.0, //
    maxx4, maxy4, minx3, maxy4, 1.0, 1.0, //
    maxx4, maxy4, minx3, maxy4, 1.0, 0.0, //
    maxx4, maxy4, minx3, maxy4, 0.0, 0.0, //

    minx3, maxy4, minx3, miny3, 0.0, 0.0, //
    minx3, maxy4, minx3, miny3, 0.0, 1.0, //
    minx3, maxy4, minx3, miny3, 1.0, 1.0, //
    minx3, maxy4, minx3, miny3, 1.0, 1.0, //
    minx3, maxy4, minx3, miny3, 1.0, 0.0, //
    minx3, maxy4, minx3, miny3, 0.0, 0.0, //
};
static const int g_shadowVertexCount = (sizeof(g_shadowVerts) / sizeof(g_shadowVerts[0])) / 6;

void
App::drawLights()
{
	glEnable(GL_BLEND);

	// A list of the visible lights we want to draw.
	struct Light
	{
		float x, y, size, radius;
		float r, g, b;
	};

	double mx, my;
	glfwGetCursorPos(m_window, &mx, &my);
	//	glm::vec2 pw = m_camera.convertScreenToWorld({mx, my});
	glm::vec2 pw{mx, my};
	const Light lights[] = {
	    {-0.5, 0.5, 1.4, 0.4, 1, 1, 0}, //
	    // {  -1,  -1,   2.5,   0.2,   1, 1, 0 },//
	    {2.0f * pw.x / m_winWidth - 1.f, 1.0f - 2.0f * pw.y / m_winHeight, 2.0, 0.2, 0, 1, 1},
	};

	const auto time = (float)glfwGetTime();
	float mat[16];
	//	transformMat4x4(mat, 0.3f * std::cos(time), 0.3f * std::sin(time), time, 1);
	transformMat4x4(mat, 0.3f, 0.3f, 0, 1);

	for (const auto &light : lights)
	{
		// Draw the shadow mask into destination alpha.
		// You can skip the transform part if you batch the geometry or something.
		// However, the shadow shader does need the light's position to know where to project from.
		m_shaderShadow.use();
		glUniformMatrix4fv(m_shaderShadow.getMatrixLoc(), 1, GL_FALSE, mat);
		glUniform3f(m_shaderShadow.getLightLoc(), light.x, light.y, light.radius);
		// Shadows should only be drawn into the alpha channel and should leave color untouched.
		// Unlike hard shadows that just black out the alpha, soft shadows are subtracted.
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_REVERSE_SUBTRACT);
		glBlendFuncSeparate(GL_ZERO, GL_ONE, GL_ONE, GL_ONE);
		glDrawArrays(GL_TRIANGLES, 0, g_shadowVertexCount);

		// This is my quick and dirty way of drawing a sprite for the lights.
		// Other than the blending mode, the implementation here is unimportant.
		float lightMat[16];
		transformMat4x4(lightMat, light.x, light.y, 0, light.size);
		m_shaderLight.use();
		glUniformMatrix4fv(m_shaderLight.getMatrixLoc(), 1, GL_FALSE, lightMat);
		glUniform3f(m_shaderLight.getColorLoc(), light.r, light.g, light.b);
		// This blend mode applies the shadow to the light, accumulates it, and resets the alpha.
		// The source color is multiplied by the destination alpha (where the shadow mask has been drawn).
		// The alpha src alpha replaces the destination alpha.
		// For the accumulate/clear trick to work your light must be opaque,
		// and cover the the whole drawable area (framebuffer or scissor rectangle)
		// TODO HDR clamp version
		glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
		glBlendFuncSeparate(GL_DST_ALPHA, GL_ONE, GL_ONE, GL_ZERO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	// At this point the light map is complete. turn off blending
	glDisable(GL_BLEND); // FIXME: blend may fail in opengl es 2.0, WTF
}

void
App::render()
{
	int prevFBO = m_framebuffer->bind();
	m_framebuffer->setSize(m_frameWidth, m_frameHeight, 0);

	// Update and render
	glViewport(0, 0, m_frameWidth, m_frameHeight);
	//		glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
	//		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClearColor(0.f, 0.f, 0.f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//	glClear(GL_COLOR_BUFFER_BIT);

	//	nvgBeginFrame(m_vg, (float)m_winWidth, (float)m_winHeight, m_devicePixelRatio);
	//	nvgSave(m_vg);
	//
	//	double xd, yd;
	//	glfwGetCursorPos(m_window, &xd, &yd);
	//	glm::vec2 pw =
	//	    m_camera.convertScreenToWorld({float(xd) / m_windowScale, float(yd) / m_windowScale});
	//	nvgBeginPath(m_vg);
	//	nvgRoundedRect(m_vg, 100, 100, 100, 200, 20);
	//	nvgCircle(m_vg, pw.x, pw.y, 3 / m_camera.getZoom());
	//	//		nvgCircle(m_vg, m_pointer.x, m_pointer.y, 3);
	//	nvgFillColor(m_vg, nvgRGBui((uint32_t)ie::Colors::RED));
	//	nvgFill(m_vg);
	//
	//	nvgRestore(m_vg);
	//	nvgEndFrame(m_vg);

	//			m_physicsWorld.debugRender();

	m_framebuffer->blit(prevFBO); // blit to prev FBO and rebind it

	if (m_enableCRT)
	{
		// add post-effect
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindTexture(GL_TEXTURE_2D, m_framebuffer->getInternalHandle());
		m_shaderCRT.use((float)m_frameWidth, (float)m_frameHeight);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
	}

	drawLights();
}

void
App::renderUI()
{
	ImGui::NewFrame();
	{
		ImGuiIO &io = ImGui::GetIO();
		const float cameraWidth = io.DisplaySize.x;
		const float cameraHeight = io.DisplaySize.y;

		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(ImVec2(cameraWidth, cameraHeight));
		ImGui::SetNextWindowBgAlpha(0.0f);
		ImGui::Begin(
		    "Overlay", nullptr,
		    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs |
		        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
		ImGui::End();

		char buffer[128];
		sprintf(buffer, "fps %.1f", 1.f / m_frameTime);
		ImGui::Begin(
		    "Overlay", nullptr,
		    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs |
		        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
		ImGui::SetCursorPos(ImVec2(5.0f, cameraHeight - 20.0f));
		ImGui::TextColored(ImColor(153, 230, 153, 255), "%s", buffer);
		ImGui::End();

		bool showDemoWindow = false;
		ImGui::ShowDemoWindow(&showDemoWindow);

		ImGui::Checkbox("enable CRT", &m_enableCRT);
	}
	ImGui::Render();
}


static constexpr char *g_crtVertexShaderSource = (char *const)
    R"vertex(
#version 330 core
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_color;
layout(location = 2) in vec2 a_texCoord;

out vec3 v_color;
out vec2 v_texCoord;

void main() {
	gl_Position = vec4(a_pos, 1.0);
	v_color = a_color;
	v_texCoord = vec2(a_texCoord.x, a_texCoord.y);
}
)vertex";
static constexpr char *g_crtFragmentShaderSource = (char *const)
    R"frag(
#version 330 core

out vec4 o_fragColor;

in vec3 v_color;
in vec2 v_texCoord;

uniform sampler2D u_texture1;
uniform float u_time;
uniform vec2 u_resolution;

// retro crt effect
vec4 CRT(vec2 uv, sampler2D tex, float curvature) {
	uv = uv * 2. - 1.;
	vec2 offset = uv.yx / curvature;
	uv += uv * offset * offset;
	uv = uv * .5 + .5;

	// distance from center of image, used to adjust blur
	float d = length(uv - vec2(0.5, 0.5));

	// blur amount
	float blur = (1.0 + sin(u_time * 0.2)) * 0.4;
	blur *= 1.0 + sin(u_time * 1.0) * 0.1;
	blur = pow(blur, 3.0);
	blur *= 0.05;
	// reduce blur towards center
	blur *= d;

	float edge_blur = 0.021;
	vec2 edge = smoothstep(0., edge_blur, uv) * (1. - smoothstep(1. - edge_blur, 1., uv));

	// chromatic aberration
	vec3 col;
	col.r = texture(tex, vec2(uv.x + blur, uv.y)).r;
	col.g = texture(tex, uv).g;
	col.b = texture(tex, vec2(uv.x - blur, uv.y)).b;
	col *= edge.x * edge.y;

	// scanline
	float scanline = sin(uv.y * u_resolution.y * 2) * 0.035;
	col -= scanline;

	// vignette
	col *= 1.0 - d * 0.5;

	return vec4(col, 1.0);
}

void main() {
	o_fragColor = CRT(v_texCoord, u_texture1, 5.9);
}
)frag";

ShaderCRT::ShaderCRT() : ie::Shader(g_crtVertexShaderSource, g_crtFragmentShaderSource)
{
	// set up vertex data (and buffer(s)) and configure vertex attributes
	float vertices[] = {
	    1.f,  1.f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, //
	    1.f,  -1.f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, //
	    -1.f, -1.f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, //
	    -1.f, 1.f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f};
	unsigned int indices[] = {0, 1, 3, 1, 2, 3};
	glGenVertexArrays(1, &m_vertexArray);
	glGenBuffers(1, &m_vertexBuffer);
	glGenBuffers(1, &m_elementBuffer);

	glBindVertexArray(m_vertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	const GLsizeiptr stride = 8 * sizeof(float);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
	glEnableVertexAttribArray(0); // position attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));
	glEnableVertexAttribArray(1); // color attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)(6 * sizeof(float)));
	glEnableVertexAttribArray(2); // texture coord attribute


	// uniform locations
	m_timeLoc = glGetUniformLocation(getProgram(), "u_time");
	m_resolutionLoc = glGetUniformLocation(getProgram(), "u_resolution");
}

void
ShaderCRT::use(float frameWidth, float frameHeight) const
{
	glUseProgram(getProgram());
	glBindVertexArray(m_vertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elementBuffer);
	glUniform1f(m_timeLoc, (float)glfwGetTime());
	glUniform2f(m_resolutionLoc, frameWidth, frameHeight);
	glBindVertexArray(m_vertexArray);
}

// Quick vertex shader to draw a light with.
// Just use a sprite in a real project, it makes the shape/gradient more flexible.
static constexpr char *g_lightVertexShaderSource = (char *const)
    R"vertex(
#version 330 core

layout(location = 0) in vec2 a_vertex;
layout(location = 1) in vec2 a_uv;

out vec2 v_uv;

uniform mat4 u_matrix;

void main() {
	gl_Position = u_matrix * vec4(a_vertex, 0, 1);
	v_uv = a_uv;
}
)vertex";

// Quick fragment shader to draw a light with.
// Just use a sprite in a real project, it makes the shape/gradient more flexible.
static constexpr char *g_lightFragmentShaderSource = (char *const)
    R"frag(
#version 330 core

in vec2 v_uv;

uniform vec3 u_color;

out vec4 fragColor;

void main() {
	// A nice radial gradient with quadratic falloff.
	float brightness = max(0.0, 1.0 - pow(dot(v_uv, v_uv), 0.25));
	fragColor = vec4(brightness * u_color, 1.0);
}
)frag";

static constexpr char *g_shadowVertexShaderSource = (char *const)
    R"vertex(
#version 330 core

layout(location = 0) in vec4 a_segment;
layout(location = 1) in vec2 a_shadow_coord;

uniform mat4 u_matrix;
uniform vec3 u_light;

out vec4 v_penumbras;
out vec3 v_edges;
out vec3 v_proj_pos;
out vec4 v_endpoints;

// column major in glsl, but row major in hlsl.
mat2 adjugate(vec2 c1, vec2 c2) { return mat2(c2[1], -c1[1], -c2[0], c1[0]); }

void main() {
	// Unpack the vertex shader input.
	vec2 endpoint_a = (u_matrix * vec4(a_segment.zw, 0.0, 1.0)).xy;
	vec2 endpoint_b = (u_matrix * vec4(a_segment.xy, 0.0, 1.0)).xy;
	vec2 endpoint = mix(endpoint_a, endpoint_b, a_shadow_coord.x);
	float light_radius = u_light.z;

	// Deltas from the segment to the light center.
	vec2 delta_a = endpoint_a - u_light.xy;
	vec2 delta_b = endpoint_b - u_light.xy;
	vec2 delta = endpoint - u_light.xy;

	// Offsets from the light center to the edge of the light volume.
	vec2 offset_a = vec2(-light_radius, light_radius) * normalize(delta_a).yx;
	vec2 offset_b = vec2(light_radius, -light_radius) * normalize(delta_b).yx;
	vec2 offset = mix(offset_a, offset_b, a_shadow_coord.x);

	// Vertex projection.
	float w = a_shadow_coord.y;
	vec3 proj_pos = vec3(mix(delta - offset, endpoint, w), w);
	gl_Position = vec4(proj_pos.xy, 0, w);

	vec2 penumbra_a = adjugate(offset_a, -delta_a) * (delta - mix(offset, delta_a, w));
	vec2 penumbra_b = adjugate(-offset_b, delta_b) * (delta - mix(offset, delta_b, w));
	v_penumbras = (light_radius > 0.0 ? vec4(penumbra_a, penumbra_b) : vec4(0, 1, 0, 1));

	// Edge values for light penetration and clipping.
	vec2 seg_delta = endpoint_b - endpoint_a;
	vec2 seg_normal = seg_delta.yx * vec2(-1.0, 1.0);
	// Calculate where the light -> pixel ray will intersect with the segment.
	v_edges.xy = -adjugate(seg_delta, delta_a + delta_b) * (delta - offset * (1.0 - w));
	v_edges.y *= 2.0; // Skip a multiply in the fragment shader.
	// Calculate a clipping coordinate that is 0 at the near edge (when w = 1)...
	// otherwise calculate the dot product with the projected coordinate.
	v_edges.z = dot(seg_normal, delta - offset) * (1.0 - w);

	// Light penetration values.
	float light_penetration = 0.01;
	v_proj_pos = vec3(proj_pos.xy, w * light_penetration);
	v_endpoints = vec4(endpoint_a, endpoint_b) / light_penetration;
}
)vertex";
static constexpr char *g_shadowFragmentShaderSource = (char *const)
    R"frag(
#version 330 core

in vec4 v_penumbras;
in vec3 v_edges;
in vec3 v_proj_pos;
in vec4 v_endpoints;

out vec4 fragColor;

void main() {
	// Calculate the light intersection point, but clamp to endpoints to avoid artifacts.
	float intersection_t = clamp(v_edges.x / abs(v_edges.y), -0.5, 0.5);
	vec2 intersection_point = (0.5 - intersection_t) * v_endpoints.xy + (0.5 + intersection_t) * v_endpoints.zw;
	// The delta from the intersection to the pixel.
	vec2 penetration_delta = intersection_point - v_proj_pos.xy / v_proj_pos.z;
	// Apply a simple falloff function.
	float bleed = min(dot(penetration_delta, penetration_delta), 1.0);

	// Penumbra mixing.
	vec2 penumbras = smoothstep(-1.0, 1.0, v_penumbras.xz / v_penumbras.yw);
	float penumbra = dot(penumbras, step(v_penumbras.yw, vec2(0.0)));
	penumbra -= 1.0 / 64.0; // Numerical precision fudge factor.

	fragColor = vec4(bleed * (1.0 - penumbra) * step(v_edges.z, 0.0));
}
)frag";

ShaderLight::ShaderLight() : ie::Shader(g_lightVertexShaderSource, g_lightFragmentShaderSource)
{
	glGenBuffers(1, &m_vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	glBufferData(
	    GL_ARRAY_BUFFER, sizeof(g_lightSpriteVerts), g_lightSpriteVerts, GL_STATIC_DRAW);
}

ShaderLight::~ShaderLight() { glDeleteBuffers(1, &m_vertexBuffer); }

void
ShaderLight::use()
{
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	GLint vertexLoc = glGetAttribLocation(getProgram(), "a_vertex");
	GLint uvLoc = glGetAttribLocation(getProgram(), "a_uv");
	glVertexAttribPointer(vertexLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
	glVertexAttribPointer(
	    uvLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
	glEnableVertexAttribArray(vertexLoc);
	glEnableVertexAttribArray(uvLoc);
	assert(vertexLoc == 0);
	assert(uvLoc == 1);

	glUseProgram(getProgram());

	m_matrixLoc = glGetUniformLocation(getProgram(), "u_matrix");
	m_colorLoc = glGetUniformLocation(getProgram(), "u_color");
}

ShaderShadow::ShaderShadow()
    : ie::Shader(g_shadowVertexShaderSource, g_shadowFragmentShaderSource)
{
	glGenBuffers(1, &m_vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_shadowVerts), g_shadowVerts, GL_STATIC_DRAW);
}

ShaderShadow::~ShaderShadow() { glDeleteBuffers(1, &m_vertexBuffer); }

void
ShaderShadow::use()
{
	glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
	GLint segLoc = glGetAttribLocation(getProgram(), "a_segment");
	GLint shadowCoordLoc = glGetAttribLocation(getProgram(), "a_shadow_coord");
	glVertexAttribPointer(segLoc, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
	glVertexAttribPointer(
	    shadowCoordLoc, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(4 * sizeof(float)));
	glEnableVertexAttribArray(segLoc);
	glEnableVertexAttribArray(shadowCoordLoc);
	assert(segLoc == 0);
	assert(shadowCoordLoc == 1);

	glUseProgram(getProgram());

	m_matrixLoc = glGetUniformLocation(getProgram(), "u_matrix");
	m_lightLoc = glGetUniformLocation(getProgram(), "u_light");
}





#include "./vg_test/demo.hpp"
#include "./vg_test/perf.hpp"
#include "./vg_test/tests.cpp"
#include "utils/VG_vtex.hpp"

int blowup = 0;
int screenshot = 0;
int premult = 0;


int swRender = 0;


void
VgApp::preload()
{
	initGraph(&m_fps, GRAPH_RENDER_FPS, "Frame Time");
	initGraph(&m_cpuGraph, GRAPH_RENDER_MS, "CPU Time");
	initGraph(&m_gpuGraph, GRAPH_RENDER_MS, "GPU Time");

	if (swRender)
	{
		m_blitter = nvgswuCreateBlitter();
		assert(m_vg);
		nvglDelete(m_vg);
		m_vg = nvgswCreate(NVG_SRGB);
		// have to set pixel format before loading any images
		nvgswSetFramebuffer(m_vg, nullptr, 800, 800, 0, 8, 16, 24);
	}

	loadDemoData(m_vg, &m_data, NVG_IMAGE_SRGB);

	initGPUTimer(&m_gpuTimer);
}

void
VgApp::cleanup()
{
	freeDemoData(m_vg, &m_data);

	if (m_blitter)
	{
		free(m_blitterFB);
		nvgswuDeleteBlitter(m_blitter);
	}

	printf("Average Frame Time: %.2f ms\n", getGraphAverage(&m_fps) * 1000.0f);
	printf("          CPU Time: %.2f ms\n", getGraphAverage(&m_cpuGraph) * 1000.0f);
	printf("          GPU Time: %.2f ms\n", getGraphAverage(&m_gpuGraph) * 1000.0f);

	if (swRender)
	{
		// FIXME nvglDelete will also destroy this ?
	}
}

void
VgApp::render()
{
	double mx, my, t, dt;
	int winWidth, winHeight;
	int fbWidth, fbHeight;
	float pxRatio;
	int prevFBO;
	float gpuTimes[3];
	int i, n;

	t = glfwGetTime();
	dt = t - prevt;
	prevt = t;

	startGPUTimer(&m_gpuTimer);

	glfwGetCursorPos(m_window, &mx, &my);
	glfwGetWindowSize(m_window, &winWidth, &winHeight);
	glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
	// Calculate pixel ration for hi-dpi devices.
	pxRatio = (float)fbWidth / (float)winWidth;

	if (swRender)
	{
		if (!m_blitterFB || fbWidth != m_blitter->width || fbHeight != m_blitter->height)
		{
			free(m_blitterFB);
			m_blitterFB = malloc(fbWidth * fbHeight * 4);
		}
		memset(m_blitterFB, 0x3F, fbWidth * fbHeight * 4);
		nvgswSetFramebuffer(m_vg, m_blitterFB, fbWidth, fbHeight, 0, 8, 16, 24);
	}
	else
	{
		prevFBO = m_framebuffer->bind();
		m_framebuffer->setSize(fbWidth, fbHeight, 0);
	}

	// Update and render
	glViewport(0, 0, fbWidth, fbHeight);
	if (premult) glClearColor(0, 0, 0, 0);
	else glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	nvgBeginFrame(m_vg, winWidth, winHeight, pxRatio);

	renderDemo(m_vg, mx, my, winWidth, winHeight, t, blowup, &m_data);

	renderGraph(m_vg, 5, 5, &m_fps);
	renderGraph(m_vg, 5 + 200 + 5, 5, &m_cpuGraph);
	if (m_gpuTimer.supported) renderGraph(m_vg, 5 + 200 + 5 + 200 + 5, 5, &m_gpuGraph);

	nvgEndFrame(m_vg);

	// Measure the CPU time taken excluding swap buffers (as the swap may wait for GPU)
	cpuTime = glfwGetTime() - t;

	updateGraph(&m_fps, dt);
	updateGraph(&m_cpuGraph, cpuTime);

	// We may get multiple results.
	n = stopGPUTimer(&m_gpuTimer, gpuTimes, 3);
	for (i = 0; i < n; i++) updateGraph(&m_gpuGraph, gpuTimes[i]);

	if (swRender)
		nvgswuBlit(m_blitter, m_blitterFB, fbWidth, fbHeight, 0, 0, fbWidth, fbHeight);
	else m_framebuffer->blit(prevFBO); // blit to prev FBO and rebind it

	if (screenshot)
	{
		screenshot = 0;
		saveScreenShot(fbWidth, fbHeight, premult, "dump.png");
	}

	glfwSwapBuffers(m_window);
	glfwPollEvents();
}

void
VgApp::onKey(int key, int scancode, int action, int mods)
{
	NVG_NOTUSED(scancode);
	NVG_NOTUSED(mods);
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(m_window, GL_TRUE);
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) blowup = !blowup;
	if (key == GLFW_KEY_S && action == GLFW_PRESS) screenshot = 1;
	if (key == GLFW_KEY_P && action == GLFW_PRESS) premult = !premult;
}
