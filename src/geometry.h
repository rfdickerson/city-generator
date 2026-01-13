#pragma once

#include <vector>
#include "math.h"

struct Polygon2D {
    std::vector<Vec2> v;

    Polygon2D Inset(float d) const;
};

struct OBB2D {
    Vec2 center;
    Vec2 axisX;
    Vec2 axisY;
    float halfX;
    float halfY;
};

Vec2 Centroid(const Polygon2D& p);
bool PointInConvexCCW(const Polygon2D& poly, Vec2 p);
float SignedArea(const std::vector<Vec2>& v);
bool PointInPolygon(const Polygon2D& poly, Vec2 p);

std::vector<unsigned> TriangulateCCW(const std::vector<Vec2>& v);

OBB2D ComputeOBB(const Polygon2D& poly);
Polygon2D PlaceRectInLot(const Polygon2D& lot, float shrink, float snapStep, Vec2 biasDir, float biasStrength);
Polygon2D PlaceRectInLot(const Polygon2D& lot, float shrink, float snapStep);
Polygon2D MakeLShapeFootprint(const Polygon2D& baseRect, float cutX, float cutY);
Polygon2D ScaleFromCentroid(const Polygon2D& p, float scale);
Polygon2D OutsetFromCentroid(const Polygon2D& p, float delta);

Polygon2D IntersectConvex(const Polygon2D& subject, const Polygon2D& clipper);
// Note: union uses convex hull of both polygons (superset for non-convex union).
Polygon2D UnionConvexHull(const Polygon2D& a, const Polygon2D& b);
