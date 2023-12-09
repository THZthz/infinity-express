#ifndef IE_MAIN_SCENE_HPP
#define IE_MAIN_SCENE_HPP

#include "World.hpp"

class MainWorld : public PhysicsWorld
{
public:
	explicit MainWorld(ie::Scene *scene) : PhysicsWorld(scene)
	{
		printf("Infinity Express v%s\n", INFINITY_EXPRESS_VERSION_STR);
		printf("\tworking directory %s\n", INFINITY_EXPRESS_WORKING_DIR);
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

	void load()
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

private:
	b2BodyId m_attachmentId = b2_nullBodyId;
	b2BodyId m_platformId = b2_nullBodyId;
	float m_speed = 0;
};

class App : public ie::Scene
{
public:
	App() : ie::Scene("Infinity Express", 800, 600), m_physicsWorld(this)
	{
		m_camera.setSize(m_winWidth, m_winHeight);
	}

	void preload() override { m_physicsWorld.initialize(); }

	void cleanup() override { m_physicsWorld.destroy(); }

	void update(float delta) override { m_physicsWorld.update(delta); }

	void render() override
	{
		//		int prevFBO = nvgluBindFramebuffer(m_framebuffer);
		//		nvgluSetFramebufferSize(m_framebuffer, m_frameWidth, m_frameHeight, 0);
		//
		//		// Update and render
		//		nvgluSetViewport(0, 0, m_frameWidth, m_frameHeight);
		//		//		nvgluClear(nvgRGBAf(0.3f, 0.3f, 0.32f, 1.0f));
		//		nvgluClear(nvgRGBAf(0.2f, 0.2f, 0.2f, 1.0f));
		//
		//
		//		nvgBeginFrame(m_vg, (float)m_winWidth, (float)m_winHeight, m_devicePixelRatio);
		//		nvgSave(m_vg);
		//
		//
		//		//		nvgBeginPath(m_vg);
		//		//		nvgRoundedRect(m_vg, 100, 100, 100, 200, 20);
		//		//		nvgCircle(m_vg, m_pointerWorld.x, m_pointerWorld.y, 3 / m_scale);
		//		//		//		nvgCircle(m_vg, m_pointer.x, m_pointer.y, 3);
		//		//		nvgFillColor(m_vg, nvgRGBui((uint32_t)ie::Colors::RED));
		//		//		nvgFill(m_vg);
		//
		//		nvgRestore(m_vg);
		//		nvgEndFrame(m_vg);
		//
		//		nvgluBlitFramebuffer(m_framebuffer, prevFBO); // blit to prev FBO and rebind it


		glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
		glViewport(0, 0, m_frameWidth, m_frameHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_physicsWorld.debugRender();
	}

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
	bool m_isRightMousePressed = false;
	ie::vec2 m_prevPos{0, 0};

private:
	MainWorld m_physicsWorld;
};


#endif // IE_MAIN_SCENE_HPP
