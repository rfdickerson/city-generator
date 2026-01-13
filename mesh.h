#pragma once

#include <vector>
#include "math.h"
#include "geometry.h"

struct Vertex {
    Vec3 pos;
    Vec4 color; // RGB = base color, A = AO
    Vec2 uv;
};

struct Mesh {
    std::vector<Vertex> v;
    std::vector<unsigned> i;
};

enum class SlabRole {
    Infrastructure,
    Podium,
    Public,
    Office,
    Terrace,
    Roof
};

void AddQuad(Mesh& m, unsigned a, unsigned b, unsigned c, unsigned d);
void AddBox(Mesh& m, Vec2 center, Vec2 axisX, Vec2 axisY,
            float halfX, float halfY, float z0, float z1, Vec3 color);

struct FloorSlab {
    Polygon2D footprint;
    float z;
    float thickness;
};

Mesh BuildSlab(const FloorSlab& s, Vec3 baseColor, float uvScale);
Mesh BuildSlab(const FloorSlab& s, Vec3 baseColor, float uvScale, SlabRole role);

Mesh BuildCurtainWall(const Polygon2D& fp, float z0, float z1, float inset,
                      Vec3 windowColor, Vec3 mullionColor,
                      float panelWidth, float mullionWidth, float uvVScale);
