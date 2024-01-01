#ifndef CANDYBOX_WORLD_HPP__
#define CANDYBOX_WORLD_HPP__

#include "candybox/vg/VG.hpp"
#include "candybox/TaskScheduler.hpp"
#include "candybox/Scene.hpp"
#include "DebugDraw.hpp"

#include "box2d/box2d.h"
#include "box2d/debug_draw.h"
#include "box2d/joint_util.h"
#include "box2d/id.h"

class PhysicsWorld
{
public:
	explicit PhysicsWorld(candybox::Scene* scene) : m_scene(scene), m_debugDraw(scene)
	{
		m_worldDebugDrawConfig = {
		    handleDrawPolygon,
		    handleDrawSolidPolygon,
		    handleDrawRoundedPolygon,
		    handleDrawCircle,
		    handleDrawSolidCircle,
		    handleDrawCapsule,
		    handleDrawSolidCapsule,
		    handleDrawSegment,
		    handleDrawTransform,
		    handleDrawPoint,
		    handleDrawString,
		    true,
		    true,
		    false,
		    false,
		    false,
		    false,
		    false,
		    false,
		    false,
		    &m_debugDraw};
	}

	virtual ~PhysicsWorld() = default;

	virtual void initialize()
	{
		uint32_t maxThreads = std::min(8u, candybox::GetNumHardwareThreads());
		m_scheduler.Initialize(maxThreads);
		m_taskCount = 0;

		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.workerCount = maxThreads;
		worldDef.enqueueTask = &enqueueTask;
		worldDef.finishTask = &finishTask;
		worldDef.userTaskContext = this;
		worldDef.enableSleep = true;
		//		worldDef.gravity = {0.0f, -10.0f};

		// These are not ideal, but useful for testing Box2D
		worldDef.bodyCapacity = 2;
		worldDef.contactCapacity = 2;
		worldDef.stackAllocatorCapacity = 0;

		m_worldId = b2CreateWorld(&worldDef);
		m_mouseJointId = b2_nullJointId;
		m_stepCount = 0;

		m_maxProfile = b2_emptyProfile;
		m_totalProfile = b2_emptyProfile;

		b2BodyDef bodyDef = b2DefaultBodyDef();
		m_groundBodyId = b2World_CreateBody(m_worldId, &bodyDef);
	}

	virtual void destroy()
	{
		// By deleting the world, we delete the bomb, mouse joint, etc.
		b2DestroyWorld(m_worldId);
	}

	void debugRender()
	{
		b2World_Draw(m_worldId, &m_worldDebugDrawConfig);
		m_debugDraw.flush();
	}

	virtual void update(float timeStep)
	{
		if (!m_scene->isRunning()) timeStep = 0.0f;

		b2World_EnableSleeping(m_worldId, m_sleeping);
		b2World_EnableWarmStarting(m_worldId, m_warmStarting);
		b2World_EnableContinuous(m_worldId, m_continuous);

		for (int32_t i = 0; i < 1; ++i)
		{
			b2World_Step(m_worldId, timeStep, m_velocityIters, m_relaxIters);
			m_taskCount = 0; // reset task count after one step of physics world's update
		}

		if (timeStep > 0.0f) { ++m_stepCount; }

		// Track maximum profile times
		using namespace candybox;
		b2Profile p = b2World_GetProfile(m_worldId);
		m_maxProfile.step = m::max(m_maxProfile.step, p.step);
		m_maxProfile.pairs = m::max(m_maxProfile.pairs, p.pairs);
		m_maxProfile.collide = m::max(m_maxProfile.collide, p.collide);
		m_maxProfile.solve = m::max(m_maxProfile.solve, p.solve);
		m_maxProfile.buildIslands = m::max(m_maxProfile.buildIslands, p.buildIslands);
		m_maxProfile.solveIslands = m::max(m_maxProfile.solveIslands, p.solveIslands);
		m_maxProfile.broadphase = m::max(m_maxProfile.broadphase, p.broadphase);
		m_maxProfile.continuous = m::max(m_maxProfile.continuous, p.continuous);

		m_totalProfile.step += p.step;
		m_totalProfile.pairs += p.pairs;
		m_totalProfile.collide += p.collide;
		m_totalProfile.solve += p.solve;
		m_totalProfile.buildIslands += p.buildIslands;
		m_totalProfile.solveIslands += p.solveIslands;
		m_totalProfile.broadphase += p.broadphase;
		m_totalProfile.continuous += p.continuous;
	}

	/*--------------------------------------------------------------------------------------*/
private:
	class Task : public candybox::ITaskSet
	{
	public:
		Task() = default;

		void ExecuteRange(candybox::TaskSetPartition range, uint32_t threadIndex) override
		{
			m_task((int32_t)range.start, (int32_t)range.end, threadIndex, m_taskContext);
		}

		b2TaskCallback* m_task = nullptr;
		void* m_taskContext = nullptr;
	};

	static void* enqueueTask(
	    b2TaskCallback* task,
	    int32_t itemCount,
	    int32_t minRange,
	    void* taskContext,
	    void* userContext)
	{
		auto* sample = static_cast<PhysicsWorld*>(userContext);
		if (sample->m_taskCount < maxTasks)
		{
			auto& sampleTask = sample->m_tasks[sample->m_taskCount];
			sampleTask.m_SetSize = itemCount;
			sampleTask.m_MinRange = minRange;
			sampleTask.m_task = task;
			sampleTask.m_taskContext = taskContext;
			sample->m_scheduler.AddTaskSetToPipe(&sampleTask);
			++sample->m_taskCount;
			return &sampleTask;
		}
		else
		{
			// This is not fatal but the maxTasks should be increased
			assert(false);
			task(0, itemCount, 0, taskContext);
			return nullptr;
		}
	}

	static void finishTask(void* taskPtr, void* userContext)
	{
		if (taskPtr != nullptr)
		{
			auto* sampleTask = static_cast<Task*>(taskPtr);
			auto* sample = static_cast<PhysicsWorld*>(userContext);
			sample->m_scheduler.WaitforTask(sampleTask);
		}
	}

	static constexpr int32_t maxTasks = 1024;
	candybox::TaskScheduler m_scheduler;
	Task m_tasks[maxTasks];
	int32_t m_taskCount = 0;

	/*--------------------------------------------------------------------------------------*/

	struct QueryContext
	{
		b2Vec2 point;
		b2BodyId bodyId;
	};

	static bool queryCallback(b2ShapeId shapeId, void* context)
	{
		auto* queryContext = static_cast<QueryContext*>(context);
		b2BodyId bodyId = b2Shape_GetBody(shapeId);
		b2BodyType bodyType = b2Body_GetType(bodyId);
		if (bodyType != b2_dynamicBody)
		{
			// continue query
			return true;
		}

		bool overlap = b2Shape_TestPoint(shapeId, queryContext->point);
		if (overlap)
		{
			// found shape
			queryContext->bodyId = bodyId;
			return false;
		}

		return true;
	}

	/*--------------------------------------------------------------------------------------*/

public:
	virtual void onMouseButton(b2Vec2 pw, int button, int action, int mods)
	{
		if (action == GLFW_PRESS)
		{
			if (B2_NON_NULL(m_mouseJointId)) return;

			if (button == GLFW_MOUSE_BUTTON_1)
			{
				// Make a small box.
				b2AABB box;
				b2Vec2 d = {0.001f, 0.001f};
				box.lowerBound = candybox::m::subv(pw, d);
				box.upperBound = candybox::m::subv(pw, d);

				// Query the world for overlapping shapes.
				QueryContext queryContext = {pw, b2_nullBodyId};
				b2World_QueryAABB(
				    m_worldId, queryCallback, box, b2_defaultQueryFilter, &queryContext);

				if (B2_NON_NULL(queryContext.bodyId))
				{
					float frequencyHz = 5.0f;
					float dampingRatio = 0.7f;
					float mass = b2Body_GetMass(queryContext.bodyId);

					b2MouseJointDef jd = b2DefaultMouseJointDef();
					jd.bodyIdA = m_groundBodyId;
					jd.bodyIdB = queryContext.bodyId;
					jd.target = pw;
					jd.maxForce = 1000.0f * mass;
					b2LinearStiffness(
					    &jd.stiffness, &jd.damping, frequencyHz, dampingRatio, m_groundBodyId,
					    queryContext.bodyId);

					m_mouseJointId = b2World_CreateMouseJoint(m_worldId, &jd);

					b2Body_Wake(queryContext.bodyId);
				}
			}
		}
		else if (action == GLFW_RELEASE)
		{
			if (B2_NON_NULL(m_mouseJointId) && button == GLFW_MOUSE_BUTTON_1)
			{
				b2World_DestroyJoint(m_mouseJointId);
				m_mouseJointId = b2_nullJointId;
			}
		}
	}

	virtual void onCursorPos(b2Vec2 pw)
	{
		if (B2_NON_NULL(m_mouseJointId))
		{
			b2MouseJoint_SetTarget(m_mouseJointId, pw);
			b2BodyId bodyIdB = b2Joint_GetBodyB(m_mouseJointId);
			b2Body_Wake(bodyIdB);
		}
	}
	/*--------------------------------------------------------------------------------------*/

private:
	/*--------------------------------------------------------------------------------------*/


	static void
	handleDrawPolygon(const b2Vec2* vertices, int vertexCount, b2Color color, void* context)
	{
		static_cast<DebugDraw*>(context)->drawPolygon(vertices, vertexCount, color);
	}

	static void handleDrawSolidPolygon(
	    const b2Vec2* vertices,
	    int vertexCount,
	    b2Color color,
	    void* context)
	{
		static_cast<DebugDraw*>(context)->drawSolidPolygon(vertices, vertexCount, color);
	}

	static void handleDrawRoundedPolygon(
	    const b2Vec2* vertices,
	    int32_t vertexCount,
	    float radius,
	    b2Color fillColor,
	    b2Color lineColor,
	    void* context)
	{
		static_cast<DebugDraw*>(context)
		    ->drawRoundedPolygon(vertices, vertexCount, radius, fillColor, lineColor);
	}

	static void handleDrawCircle(b2Vec2 center, float radius, b2Color color, void* context)
	{
		static_cast<DebugDraw*>(context)->drawCircle(center, radius, color);
	}

	static void handleDrawSolidCircle(
	    b2Vec2 center,
	    float radius,
	    b2Vec2 axis,
	    b2Color color,
	    void* context)
	{
		static_cast<DebugDraw*>(context)->drawSolidCircle(center, radius, axis, color);
	}

	static void
	handleDrawCapsule(b2Vec2 p1, b2Vec2 p2, float radius, b2Color color, void* context)
	{
		static_cast<DebugDraw*>(context)->drawCapsule(p1, p2, radius, color);
	}

	static void
	handleDrawSolidCapsule(b2Vec2 p1, b2Vec2 p2, float radius, b2Color color, void* context)
	{
		static_cast<DebugDraw*>(context)->drawSolidCapsule(p1, p2, radius, color);
	}

	static void handleDrawSegment(b2Vec2 p1, b2Vec2 p2, b2Color color, void* context)
	{
		static_cast<DebugDraw*>(context)->drawSegment(p1, p2, color);
	}

	static void handleDrawTransform(b2Transform xf, void* context)
	{
		static_cast<DebugDraw*>(context)->drawTransform(xf);
	}

	static void handleDrawPoint(b2Vec2 p, float size, b2Color color, void* context)
	{
		static_cast<DebugDraw*>(context)->drawPoint(p, size, color);
	}

	static void handleDrawString(b2Vec2 p, const char* s, void* context)
	{
		static_cast<DebugDraw*>(context)->drawString(p, s);
	}

	/*--------------------------------------------------------------------------------------*/

protected:
	b2BodyId m_groundBodyId = b2_nullBodyId;
	b2WorldId m_worldId = b2_nullWorldId;
	b2JointId m_mouseJointId = b2_nullJointId;
	int32_t m_stepCount = 0;
	b2Profile m_maxProfile = b2_emptyProfile;
	b2Profile m_totalProfile = b2_emptyProfile;

	int m_velocityIters = 8;
	int m_relaxIters = 4;
	bool m_sleeping = true;
	bool m_warmStarting = true;
	bool m_continuous = true;

	DebugDraw m_debugDraw;
	b2DebugDraw m_worldDebugDrawConfig{};

	candybox::Scene* m_scene;
};

#endif // CANDYBOX_WORLD_HPP__
