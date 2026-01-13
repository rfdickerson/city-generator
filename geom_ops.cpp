#include "geom_ops.h"

#include <algorithm>
#include <cmath>

Polygon2D Inset(const Polygon2D& poly, float d)
{
    return poly.Inset(d);
}

Polygon2D MakeRectangle(Vec2 center, float width, float depth, float angleRadians)
{
    float hw = width * 0.5f;
    float hd = depth * 0.5f;
    float c = std::cos(angleRadians);
    float s = std::sin(angleRadians);

    Vec2 axisX{c, s};
    Vec2 axisY{-s, c};

    Vec2 ax = axisX * hw;
    Vec2 ay = axisY * hd;

    // CCW rectangle.
    Polygon2D rect {{
        center + ax + ay,
        center - ax + ay,
        center - ax - ay,
        center + ax - ay
    }};
    return rect;
}

Mesh Extrude(const Polygon2D& poly, float z0, float z1)
{
    if(z1 < z0){
        std::swap(z0, z1);
    }
    float thickness = z1 - z0;
    Vec3 color{0.6f, 0.6f, 0.6f};
    return BuildSlab({poly, z0, thickness}, color, 0.02f);
}

Mesh MakeBox(Vec3 min, Vec3 max)
{
    Mesh m;
    unsigned base = 0;
    Vec2 uv{0,0};
    Vec4 col{0.6f,0.6f,0.6f,1.0f};

    m.v.push_back({{min.x,min.y,min.z},col,uv}); // 0
    m.v.push_back({{max.x,min.y,min.z},col,uv}); // 1
    m.v.push_back({{max.x,min.y,max.z},col,uv}); // 2
    m.v.push_back({{min.x,min.y,max.z},col,uv}); // 3
    m.v.push_back({{min.x,max.y,min.z},col,uv}); // 4
    m.v.push_back({{max.x,max.y,min.z},col,uv}); // 5
    m.v.push_back({{max.x,max.y,max.z},col,uv}); // 6
    m.v.push_back({{min.x,max.y,max.z},col,uv}); // 7

    AddQuad(m,base+0,base+1,base+2,base+3); // bottom
    AddQuad(m,base+4,base+7,base+6,base+5); // top
    AddQuad(m,base+0,base+4,base+5,base+1);
    AddQuad(m,base+1,base+5,base+6,base+2);
    AddQuad(m,base+2,base+6,base+7,base+3);
    AddQuad(m,base+3,base+7,base+4,base+0);

    return m;
}

void Append(Mesh& dst, const Mesh& src)
{
    unsigned offset = (unsigned)dst.v.size();
    dst.v.insert(dst.v.end(), src.v.begin(), src.v.end());
    dst.i.reserve(dst.i.size() + src.i.size());
    for(unsigned idx : src.i){
        dst.i.push_back(idx + offset);
    }
}
