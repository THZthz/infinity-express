#ifndef IE_DEBUG_DRAW_HPP
#define IE_DEBUG_DRAW_HPP

#include <cassert>
#include "box2d/box2d.h"
#include "box2d/debug_draw.h"
#include "box2d/id.h"
#include "nanovg.h"

class DebugDraw
{
public:
	DebugDraw(NVGcontext* vg) : m_vg(vg)
	{
		assert(m_vg);
		m_debugDraw = {
		    DrawPolygonFcn,
		    DrawSolidPolygonFcn,
		    DrawRoundedPolygonFcn,
		    DrawCircleFcn,
		    DrawSolidCircleFcn,
		    DrawCapsuleFcn,
		    DrawSolidCapsuleFcn,
		    DrawSegmentFcn,
		    DrawTransformFcn,
		    DrawPointFcn,
		    DrawStringFcn,
		    true,
		    true,
		    false,
		    false,
		    false,
		    this};
	}

	void draw(b2WorldId worldId) { b2World_Draw(worldId, &m_debugDraw); }

public:
	void DrawPolygon(const b2Vec2* vertices, int32_t vertexCount, b2Color color)
	{
		nvgBeginPath(m_vg);
		nvgMoveTo(m_vg, vertices[0].x, vertices[0].y);
		for (int32_t i = 1; i < vertexCount; ++i)
			nvgLineTo(m_vg, vertices[i].x, vertices[i].y);
		nvgClosePath(m_vg);
		nvgStrokeColor(m_vg, nvgRGBAf(color.r, color.g, color.b, color.a));
		nvgStroke(m_vg);
	}

	void DrawSolidPolygon(const b2Vec2* vertices, int32_t vertexCount, b2Color color)
	{
		nvgBeginPath(m_vg);
		nvgMoveTo(m_vg, vertices[0].x, vertices[0].y);
		for (int32_t i = 1; i < vertexCount; ++i)
			nvgLineTo(m_vg, vertices[i].x, vertices[i].y);
		nvgClosePath(m_vg);
		nvgFillColor(m_vg, nvgTransRGBAf(nvgRGBAf(color.r, color.g, color.b, color.a), 0.5f));
		nvgFill(m_vg);
	}

	void DrawRoundedPolygon(
	    const b2Vec2* vertices,
	    int32_t vertexCount,
	    float radius,
	    b2Color fillColor,
	    b2Color outlineColor)
	{
		nvgBeginPath(m_vg);
		nvgRoundPolygon(
		    m_vg, vertexCount, const_cast<float*>(reinterpret_cast<const float*>(vertices)),
		    radius);
		nvgFillColor(
		    m_vg,
		    nvgTransRGBAf(nvgRGBAf(fillColor.r, fillColor.g, fillColor.b, fillColor.a), 0.5f));
		nvgFill(m_vg);
	}

	void DrawCircle(b2Vec2 center, float radius, b2Color color)
	{
		nvgBeginPath(m_vg);
		nvgCircle(m_vg, center.x, center.y, radius);
		nvgStrokeColor(m_vg, nvgRGBAf(color.r, color.g, color.b, color.a));
		nvgStroke(m_vg);
	}

	void DrawSolidCircle(b2Vec2 center, float radius, b2Vec2 axis, b2Color color)
	{
		nvgBeginPath(m_vg);
		nvgCircle(m_vg, center.x, center.y, radius);
		nvgFillColor(m_vg, nvgTransRGBAf(nvgRGBAf(color.r, color.g, color.b, color.a), 0.5f));
		nvgFill(m_vg);
	}

	void DrawCapsule(b2Vec2 p1, b2Vec2 p2, float radius, b2Color color)
	{
		nvgBeginPath(m_vg);
		nvgCapsule(m_vg, p1.x, p1.y, p2.x, p2.y, radius);
		nvgStrokeColor(m_vg, nvgRGBAf(color.r, color.g, color.b, color.a));
		nvgStroke(m_vg);
	}

	void DrawSolidCapsule(b2Vec2 p1, b2Vec2 p2, float radius, b2Color color)
	{
		nvgBeginPath(m_vg);
		nvgCapsule(m_vg, p1.x, p1.y, p2.x, p2.y, radius);
		nvgFillColor(m_vg, nvgTransRGBAf(nvgRGBAf(color.r, color.g, color.b, color.a), 0.5f));
		nvgFill(m_vg);
	}

	void DrawSegment(b2Vec2 p1, b2Vec2 p2, b2Color color)
	{
		nvgBeginPath(m_vg);
		nvgSegment(m_vg, p1.x, p1.y, p2.x, p2.y);
		nvgStrokeColor(m_vg, nvgRGBAf(color.r, color.g, color.b, color.a));
		nvgStroke(m_vg);
	}

	void DrawTransform(b2Transform xf) { }

	void DrawPoint(b2Vec2 p, float size, b2Color color) { }

	void DrawString(int x, int y, const char* string, ...) { }

	void DrawString(b2Vec2 p, const char* string, ...) { }

	void DrawAABB(b2AABB aabb, b2Color color) { }

	void Flush() { }

private:
	static void
	DrawPolygonFcn(const b2Vec2* vertices, int vertexCount, b2Color color, void* context)
	{
		static_cast<DebugDraw*>(context)->DrawPolygon(vertices, vertexCount, color);
	}

	static void
	DrawSolidPolygonFcn(const b2Vec2* vertices, int vertexCount, b2Color color, void* context)
	{
		static_cast<DebugDraw*>(context)->DrawSolidPolygon(vertices, vertexCount, color);
	}

	static void DrawRoundedPolygonFcn(
	    const b2Vec2* vertices,
	    int32_t vertexCount,
	    float radius,
	    b2Color fillColor,
	    b2Color lineColor,
	    void* context)
	{
		static_cast<DebugDraw*>(context)
		    ->DrawRoundedPolygon(vertices, vertexCount, radius, fillColor, lineColor);
	}

	static void DrawCircleFcn(b2Vec2 center, float radius, b2Color color, void* context)
	{
		static_cast<DebugDraw*>(context)->DrawCircle(center, radius, color);
	}

	static void
	DrawSolidCircleFcn(b2Vec2 center, float radius, b2Vec2 axis, b2Color color, void* context)
	{
		static_cast<DebugDraw*>(context)->DrawSolidCircle(center, radius, axis, color);
	}

	static void
	DrawCapsuleFcn(b2Vec2 p1, b2Vec2 p2, float radius, b2Color color, void* context)
	{
		static_cast<DebugDraw*>(context)->DrawCapsule(p1, p2, radius, color);
	}

	static void
	DrawSolidCapsuleFcn(b2Vec2 p1, b2Vec2 p2, float radius, b2Color color, void* context)
	{
		static_cast<DebugDraw*>(context)->DrawSolidCapsule(p1, p2, radius, color);
	}

	static void DrawSegmentFcn(b2Vec2 p1, b2Vec2 p2, b2Color color, void* context)
	{
		static_cast<DebugDraw*>(context)->DrawSegment(p1, p2, color);
	}

	static void DrawTransformFcn(b2Transform xf, void* context)
	{
		static_cast<DebugDraw*>(context)->DrawTransform(xf);
	}

	static void DrawPointFcn(b2Vec2 p, float size, b2Color color, void* context)
	{
		static_cast<DebugDraw*>(context)->DrawPoint(p, size, color);
	}

	static void DrawStringFcn(b2Vec2 p, const char* s, void* context)
	{
		static_cast<DebugDraw*>(context)->DrawString(p, s);
	}

private:
	NVGcontext* m_vg;
	b2DebugDraw m_debugDraw;
};

#endif // IE_DEBUG_DRAW_HPP
