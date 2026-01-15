#pragma once

#include "geometry.h"
#include "mesh.h"

// 2D
Polygon2D Inset(const Polygon2D& poly, float d);
Polygon2D MakeRectangle(Vec2 center, float width, float depth, float angleRadians);
Polygon2D FitRectToSize(const Polygon2D& rect, float targetWidth, float targetDepth);

// 3D
Mesh Extrude(const Polygon2D& poly, float z0, float z1);
Mesh MakeBox(Vec3 min, Vec3 max);

// Composition
void Append(Mesh& dst, const Mesh& src);
