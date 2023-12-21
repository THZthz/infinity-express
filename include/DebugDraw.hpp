#ifndef IE_DEBUG_DRAW_HPP
#define IE_DEBUG_DRAW_HPP

#include <cassert>
#include "box2d/box2d.h"
#include "box2d/debug_draw.h"
#include "box2d/id.h"
#include "candybox/VG.hpp"
#include "candybox/Scene.hpp"

class DebugDraw
{
public:
	explicit DebugDraw(ie::Scene* scene);
	~DebugDraw();


public:
	void drawPolygon(const b2Vec2* vertices, int32_t vertexCount, b2Color color);
	void drawSolidPolygon(const b2Vec2* vertices, int32_t vertexCount, b2Color color);
	void drawRoundedPolygon(
	    const b2Vec2* vertices,
	    int32_t vertexCount,
	    float radius,
	    b2Color fillColor,
	    b2Color outlineColor);
	void drawCircle(b2Vec2 center, float radius, b2Color color);
	void drawSolidCircle(b2Vec2 center, float radius, b2Vec2 axis, b2Color color);
	void drawCapsule(b2Vec2 p1, b2Vec2 p2, float radius, b2Color color);
	void drawSolidCapsule(b2Vec2 p1, b2Vec2 p2, float radius, b2Color color);
	void drawSegment(b2Vec2 p1, b2Vec2 p2, b2Color color);
	void drawTransform(b2Transform xf);
	void drawPoint(b2Vec2 p, float size, b2Color color);
	void drawString(int x, int y, const char* string, ...);
	void drawString(b2Vec2 p, const char* string, ...);
	void drawAABB(b2AABB aabb, b2Color color);
	void flush();

private:
	ie::Scene* m_scene;

	struct GLRenderPoints* m_points;
	struct GLRenderLines* m_lines;
	struct GLRenderTriangles* m_triangles;
	struct GLRenderRoundedTriangles* m_roundedTriangles;

	 ie::Scene::Camera *m_camera;
};

#endif // IE_DEBUG_DRAW_HPP
