#ifndef IE_MAIN_SCENE_HPP
#define IE_MAIN_SCENE_HPP

#include "Physics/World.hpp"

class MainWorld : public PhysicsWorld
{
public:
	explicit MainWorld(ie::Scene *scene) : PhysicsWorld(scene) { }

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
	App() : ie::Scene("Infinity Express", 1000, 600), m_physicsWorld(this) { m_scale = 10; }

	void preload() override { m_physicsWorld.initialize(); }

	void cleanup() override { m_physicsWorld.destroy(); }

	void update(float delta) override { m_physicsWorld.update(); }

	void render() override
	{
		int prevFBO = nvgluBindFramebuffer(m_framebuffer);
		nvgluSetFramebufferSize(m_framebuffer, m_frameWidth, m_frameHeight, 0);

		// Update and render
		nvgluSetViewport(0, 0, m_frameWidth, m_frameHeight);
		//		nvgluClear(nvgRGBAf(0.3f, 0.3f, 0.32f, 1.0f));
		nvgluClear(nvgRGBAf(0.2f, 0.2f, 0.2f, 1.0f));


		nvgBeginFrame(m_vg, (float)m_winWidth, (float)m_winHeight, m_devicePixelRatio);
		nvgSave(m_vg);
		nvgTransform(m_vg, m_transform);

		m_physicsWorld.debugRender();

		nvgBeginPath(m_vg);
		nvgRoundedRect(m_vg, 100, 100, 100, 200, 20);
		nvgCircle(m_vg, m_pointerWorld.x, m_pointerWorld.y, 3 / m_scale);
		//		nvgCircle(m_vg, m_pointer.x, m_pointer.y, 3);
		nvgFillColor(m_vg, nvgRGBui((uint32_t)ie::Colors::RED));
		nvgFill(m_vg);
		nvgRestore(m_vg);
		nvgEndFrame(m_vg);

		nvgluBlitFramebuffer(m_framebuffer, prevFBO); // blit to prev FBO and rebind it
	}

	// ----------------------------------------------------------------
public:
	void onKey(int key, int scancode, int action, int mods) override { }

	void onMouseButton(int button, int action, int mods) override
	{
		m_physicsWorld.onMouseButton(button, action, mods);

		if (button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			if (action == GLFW_PRESS)
			{
				m_isRightMousePressed = true;
				m_prevPos = m_pointer;
			}
			else { m_isRightMousePressed = false; }
		}
	}

	void onCursorPos(double x, double y) override
	{
		m_physicsWorld.onCursorPos();

		if (m_isRightMousePressed)
		{
			m_translation = m_translation + (m_pointer - m_prevPos);
			m_prevPos = m_pointer;
		}
	}

	void onScroll(double x, double y) override
	{
		if (y > 0)
		{
			//			m_scale += 0.12f;
			m_scale *= 1.1f;
		}
		else if (y < 0)
		{
			//			m_scale -= 0.12f;
			m_scale *= 0.9f;
		}
	}

private:
	bool m_isRightMousePressed = false;
	ie::vec2 m_prevPos{0, 0};

private:
	MainWorld m_physicsWorld;
};


#endif // IE_MAIN_SCENE_HPP
