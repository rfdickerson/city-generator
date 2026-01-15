#include "building_compiler.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "geom_ops.h"
#include "geometry.h"

namespace {

struct Frame2D {
    Vec2 origin;
    Vec2 axisU;
    Vec2 axisV;
};

float Clamp01(float v)
{
    return std::max(0.0f, std::min(1.0f, v));
}

float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

Frame2D ComputeRoofFrame(const Polygon2D& footprint)
{
    OBB2D obb = ComputeOBB(footprint);
    Vec2 axisU = (obb.halfX >= obb.halfY) ? obb.axisX : obb.axisY;
    axisU = Normalize(axisU);
    Vec2 axisV = Normalize(Perp(axisU));
    return {obb.center, axisU, axisV};
}

Vec2 ToLocal(const Frame2D& frame, Vec2 p)
{
    Vec2 d = p - frame.origin;
    return {Dot(d, frame.axisU), Dot(d, frame.axisV)};
}

float PointSegmentDistance(Vec2 p, Vec2 a, Vec2 b)
{
    Vec2 ab{b.x - a.x, b.y - a.y};
    float abLen2 = Dot(ab, ab);
    if(abLen2 <= 1e-6f){
        return Length({p.x - a.x, p.y - a.y});
    }
    float t = Dot({p.x - a.x, p.y - a.y}, ab) / abLen2;
    t = Clamp01(t);
    Vec2 proj{a.x + ab.x * t, a.y + ab.y * t};
    return Length({p.x - proj.x, p.y - proj.y});
}

float MinEdgeDistance(const Polygon2D& poly, Vec2 p)
{
    float minDist = std::numeric_limits<float>::max();
    int n = (int)poly.v.size();
    for(int i = 0; i < n; ++i){
        Vec2 a = poly.v[i];
        Vec2 b = poly.v[(i + 1) % n];
        minDist = std::min(minDist, PointSegmentDistance(p, a, b));
    }
    return minDist;
}

float HeightGable(float vAbs, float ridgeHeight, float slope)
{
    return std::max(0.0f, ridgeHeight - slope * vAbs);
}

float HeightHip(float uAbs, float vAbs, float ridgeHalfLen, float ridgeHeight, float slope)
{
    float du = std::max(0.0f, uAbs - ridgeHalfLen);
    float d = std::max(vAbs, du);
    return std::max(0.0f, ridgeHeight - slope * d);
}

Mesh BuildRoofMesh(const RoofVolume& roof)
{
    Mesh m;
    if(roof.footprint.v.size() < 3){
        return m;
    }

    Polygon2D roofFootprint = roof.footprint;
    if(roof.params.overhang > 1e-4f){
        roofFootprint = OutsetFromCentroid(roofFootprint, roof.params.overhang);
    }

    Frame2D frame = ComputeRoofFrame(roofFootprint);
    float slope = std::tan(roof.params.pitchDeg * 3.14159265f / 180.0f);

    float maxAbsU = 0.0f;
    float maxAbsV = 0.0f;
    for(const auto& p : roofFootprint.v){
        Vec2 local = ToLocal(frame, p);
        maxAbsU = std::max(maxAbsU, std::fabs(local.x));
        maxAbsV = std::max(maxAbsV, std::fabs(local.y));
    }
    float ridgeHalfLen = maxAbsU * Clamp01(roof.params.hipRidgeFrac);
    float edgeScale = std::max(0.5f, std::min(maxAbsU, maxAbsV));

    std::vector<unsigned> tris = TriangulateCCW(roofFootprint.v);
    for(size_t k = 0; k + 2 < tris.size(); k += 3){
        Vec2 p0 = roofFootprint.v[tris[k]];
        Vec2 p1 = roofFootprint.v[tris[k + 1]];
        Vec2 p2 = roofFootprint.v[tris[k + 2]];
        Vec2 pts[3] = {p0, p1, p2};

        for(int i = 0; i < 3; ++i){
            Vec2 p = pts[i];
            Vec2 local = ToLocal(frame, p);
            float uAbs = std::fabs(local.x);
            float vAbs = std::fabs(local.y);
            float height = (roof.params.type == RoofType::Hip)
                               ? HeightHip(uAbs, vAbs, ridgeHalfLen, roof.params.ridgeHeight, slope)
                               : HeightGable(vAbs, roof.params.ridgeHeight, slope);
            float edgeDist01 = Clamp01(MinEdgeDistance(roofFootprint, p) / edgeScale);
            float ao = 0.70f + 0.30f * edgeDist01;
            ao = Lerp(1.0f, ao, Clamp01(roof.params.aoStrength));

            m.v.push_back({
                {p.x, roof.baseZ + height, p.y},
                {roof.color.x, roof.color.y, roof.color.z, ao},
                {p.x * roof.uvScale, p.y * roof.uvScale}
            });
        }

        unsigned base = (unsigned)m.v.size() - 3;
        m.i.insert(m.i.end(), {base, base + 1, base + 2});
    }

    return m;
}

} // namespace

Mesh CompileBuildingModel(const BuildingModel& model)
{
    Mesh out;
    for(const auto& slab : model.slabs){
        Append(out, BuildSlab({slab.footprint, slab.z, slab.thickness},
                              slab.color,
                              slab.uvScale,
                              slab.role));
    }
    for(const auto& curtain : model.curtains){
        Append(out, BuildCurtainWall(curtain.footprint,
                                     curtain.z0,
                                     curtain.z1,
                                     curtain.inset,
                                     curtain.windowColor,
                                     curtain.mullionColor,
                                     curtain.panelWidth,
                                     curtain.mullionWidth,
                                     curtain.uvVScale));
    }
    for(const auto& box : model.boxes){
        AddBox(out, box.center, box.axisX, box.axisY, box.halfX, box.halfY, box.z0, box.z1, box.color);
    }
    for(const auto& roof : model.roofs){
        Append(out, BuildRoofMesh(roof));
    }
    return out;
}
