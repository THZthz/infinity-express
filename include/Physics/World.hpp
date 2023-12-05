#ifndef IE_WORLD_HPP
#define IE_WORLD_HPP

#include "box2d/id.h"
#include "utils/TaskScheduler.hpp"
#include "utils/Scene.hpp"
#include "box2d/box2d.h"
#include "box2d/joint_util.h"
#include "nanovg.h"

class PhysicsWorld
{
public:
	explicit PhysicsWorld(ie::Scene* scene) : m_scene(scene), m_debugDraw(scene->getVg()) { }

	virtual ~PhysicsWorld() = default;

	virtual void initialize()
	{
		uint32_t maxThreads = std::min(8u, ie::GetNumHardwareThreads());
		m_scheduler.Initialize(maxThreads);
		m_taskCount = 0;

		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.workerCount = maxThreads;
		worldDef.enqueueTask = &enqueueTask;
		worldDef.finishTask = &finishTask;
		worldDef.finishAllTasks = &finishAllTasks;
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
		nvgStrokeWidth(m_scene->getVg(), 1.f / m_scene->getScale());
		m_debugDraw.draw(m_worldId);
	}

	virtual void update()
	{
		float timeStep = m_scene->getFreq() > 0.0f ? 1.0f / m_scene->getFreq() : 0.0f;
		if (!m_scene->isRunning()) timeStep = 0.0f;

		b2World_EnableSleeping(m_worldId, m_sleeping);
		b2World_EnableWarmStarting(m_worldId, m_warmStarting);
		b2World_EnableContinuous(m_worldId, m_continuous);

		for (int32_t i = 0; i < 1; ++i)
		{
			b2World_Step(m_worldId, timeStep, m_velocityIters, m_relaxIters);
		}

		if (timeStep > 0.0f) { ++m_stepCount; }

		// Track maximum profile times
		b2Profile p = b2World_GetProfile(m_worldId);
		m_maxProfile.step = ie::Max(m_maxProfile.step, p.step);
		m_maxProfile.pairs = ie::Max(m_maxProfile.pairs, p.pairs);
		m_maxProfile.collide = ie::Max(m_maxProfile.collide, p.collide);
		m_maxProfile.solve = ie::Max(m_maxProfile.solve, p.solve);
		m_maxProfile.buildIslands = ie::Max(m_maxProfile.buildIslands, p.buildIslands);
		m_maxProfile.solveIslands = ie::Max(m_maxProfile.solveIslands, p.solveIslands);
		m_maxProfile.broadphase = ie::Max(m_maxProfile.broadphase, p.broadphase);
		m_maxProfile.continuous = ie::Max(m_maxProfile.continuous, p.continuous);

		m_totalProfile.step += p.step;
		m_totalProfile.pairs += p.pairs;
		m_totalProfile.collide += p.collide;
		m_totalProfile.solve += p.solve;
		m_totalProfile.buildIslands += p.buildIslands;
		m_totalProfile.solveIslands += p.solveIslands;
		m_totalProfile.broadphase += p.broadphase;
		m_totalProfile.continuous += p.continuous;
	}

	// ----------------------------------------------------------------
public:
	virtual void onMouseButton(int button, int action, int mods)
	{
		if (action == GLFW_PRESS)
		{
			if (B2_NON_NULL(m_mouseJointId)) return;

			if (button == GLFW_MOUSE_BUTTON_1)
			{
				// Make a small box.
				b2AABB box;
				b2Vec2 d = {0.001f, 0.001f};
				b2Vec2 p = {m_scene->getPointerWorld().x, m_scene->getPointerWorld().y};
				box.lowerBound = ie::SubV(p, d);
				box.upperBound = ie::AddV(p, d);

				// Query the world for overlapping shapes.
				QueryContext queryContext = {p, b2_nullBodyId};
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
					jd.target = p;
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

	virtual void onCursorPos()
	{
		if (B2_NON_NULL(m_mouseJointId))
		{
			b2MouseJoint_SetTarget(
			    m_mouseJointId, {m_scene->getPointerWorld().x, m_scene->getPointerWorld().y});
			b2BodyId bodyIdB = b2Joint_GetBodyB(m_mouseJointId);
			b2Body_Wake(bodyIdB);
		}
	}

	// ----------------------------------------------------------------
private:
	class SampleTask : public ie::ITaskSet
	{
	public:
		SampleTask() = default;

		void ExecuteRange(ie::TaskSetPartition range, uint32_t threadIndex) override
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
			SampleTask& sampleTask = sample->m_tasks[sample->m_taskCount];
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
			assert(false);
			task(0, itemCount, 0, taskContext);
			return nullptr;
		}
	}

	static void finishTask(void* taskPtr, void* userContext)
	{
		auto* sampleTask = static_cast<SampleTask*>(taskPtr);
		auto* sample = static_cast<PhysicsWorld*>(userContext);
		sample->m_scheduler.WaitforTask(sampleTask);
	}

	static void finishAllTasks(void* userContext)
	{
		auto* sample = static_cast<PhysicsWorld*>(userContext);
		sample->m_scheduler.WaitforAll();
		sample->m_taskCount = 0;
	}

	static constexpr int32_t maxTasks = 1024;
	ie::TaskScheduler m_scheduler;
	SampleTask m_tasks[maxTasks];
	int32_t m_taskCount = 0;

	// ----------------------------------------------------------------

	struct QueryContext
	{
		b2Vec2 point{};
		b2BodyId bodyId = b2_nullBodyId;
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

	// ----------------------------------------------------------------

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

	ie::Scene* m_scene;
};

#endif // IE_WORLD_HPP
