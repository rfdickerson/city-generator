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

struct SplitPolys {
    Polygon2D pos;
    Polygon2D neg;
    bool hasPos = false;
    bool hasNeg = false;
};

SplitPolys SplitPolygonByV0(const Polygon2D& poly, const Frame2D& frame)
{
    std::vector<Vec2> pos;
    std::vector<Vec2> neg;
    int n = (int)poly.v.size();
    if(n < 3){
        return {};
    }

    for(int i = 0; i < n; ++i){
        Vec2 a = poly.v[i];
        Vec2 b = poly.v[(i + 1) % n];
        float va = ToLocal(frame, a).y;
        float vb = ToLocal(frame, b).y;

        if(va >= 0.0f){
            pos.push_back(a);
        }
        if(va <= 0.0f){
            neg.push_back(a);
        }

        if((va > 0.0f && vb < 0.0f) || (va < 0.0f && vb > 0.0f)){
            float t = va / (va - vb);
            Vec2 hit{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
            pos.push_back(hit);
            neg.push_back(hit);
        }
    }

    SplitPolys out;
    if(pos.size() >= 3){
        out.pos = {pos};
        out.hasPos = true;
    }
    if(neg.size() >= 3){
        out.neg = {neg};
        out.hasNeg = true;
    }
    return out;
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

float ComputeRoofHeight(const RoofVolume& roof,
                        const Frame2D& frame,
                        float ridgeHalfLen,
                        float slope,
                        Vec2 p)
{
    Vec2 local = ToLocal(frame, p);
    float uAbs = std::fabs(local.x);
    float vAbs = std::fabs(local.y);
    return (roof.params.type == RoofType::Hip)
               ? HeightHip(uAbs, vAbs, ridgeHalfLen, roof.params.ridgeHeight, slope)
               : HeightGable(vAbs, roof.params.ridgeHeight, slope);
}

float ComputeRoofAo(const RoofVolume& roof, const Polygon2D& footprint, float edgeScale, Vec2 p)
{
    float edgeDist01 = Clamp01(MinEdgeDistance(footprint, p) / edgeScale);
    float ao = 0.70f + 0.30f * edgeDist01;
    return Lerp(1.0f, ao, Clamp01(roof.params.aoStrength));
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

    auto emitTriangles = [&](const Polygon2D& poly){
        std::vector<unsigned> tris = TriangulateCCW(poly.v);
        for(size_t k = 0; k + 2 < tris.size(); k += 3){
            Vec2 p0 = poly.v[tris[k]];
            Vec2 p1 = poly.v[tris[k + 1]];
            Vec2 p2 = poly.v[tris[k + 2]];
            Vec2 pts[3] = {p0, p1, p2};

            for(int i = 0; i < 3; ++i){
                Vec2 p = pts[i];
                float height = ComputeRoofHeight(roof, frame, ridgeHalfLen, slope, p);
                float ao = ComputeRoofAo(roof, roofFootprint, edgeScale, p);

                m.v.push_back({
                    {p.x, roof.baseZ + height, p.y},
                    {roof.color.x, roof.color.y, roof.color.z, ao},
                    {p.x * roof.uvScale, p.y * roof.uvScale}
                });
            }

            unsigned base = (unsigned)m.v.size() - 3;
            m.i.insert(m.i.end(), {base, base + 1, base + 2});
        }
    };

    if(roof.params.type == RoofType::Gable){
        SplitPolys split = SplitPolygonByV0(roofFootprint, frame);
        if(split.hasPos) emitTriangles(split.pos);
        if(split.hasNeg) emitTriangles(split.neg);
        if(!split.hasPos && !split.hasNeg){
            emitTriangles(roofFootprint);
        }
    }else{
        emitTriangles(roofFootprint);
    }

    int n = (int)roofFootprint.v.size();
    float wallAo = Lerp(1.0f, 0.65f, Clamp01(roof.params.aoStrength));
    for(int i = 0; i < n; ++i){
        int j = (i + 1) % n;
        Vec2 a = roofFootprint.v[i];
        Vec2 b = roofFootprint.v[j];
        float ha = ComputeRoofHeight(roof, frame, ridgeHalfLen, slope, a);
        float hb = ComputeRoofHeight(roof, frame, ridgeHalfLen, slope, b);

        unsigned base = (unsigned)m.v.size();
        m.v.push_back({{a.x, roof.baseZ, a.y}, {roof.color.x, roof.color.y, roof.color.z, wallAo},
                       {a.x * roof.uvScale, a.y * roof.uvScale}});
        m.v.push_back({{b.x, roof.baseZ, b.y}, {roof.color.x, roof.color.y, roof.color.z, wallAo},
                       {b.x * roof.uvScale, b.y * roof.uvScale}});
        m.v.push_back({{b.x, roof.baseZ + hb, b.y}, {roof.color.x, roof.color.y, roof.color.z, wallAo},
                       {b.x * roof.uvScale, b.y * roof.uvScale}});
        m.v.push_back({{a.x, roof.baseZ + ha, a.y}, {roof.color.x, roof.color.y, roof.color.z, wallAo},
                       {a.x * roof.uvScale, a.y * roof.uvScale}});
        AddQuad(m, base + 0, base + 1, base + 2, base + 3);
    }

    Polygon2D baseFootprint = roof.baseFootprint;
    if(baseFootprint.v.size() < 3){
        baseFootprint = roofFootprint;
    }
    int bn = (int)baseFootprint.v.size();
    float capAo = Lerp(1.0f, 0.55f, Clamp01(roof.params.aoStrength));
    for(int i = 0; i < bn; ++i){
        int j = (i + 1) % bn;
        Vec2 a = baseFootprint.v[i];
        Vec2 b = baseFootprint.v[j];
        float ha = ComputeRoofHeight(roof, frame, ridgeHalfLen, slope, a);
        float hb = ComputeRoofHeight(roof, frame, ridgeHalfLen, slope, b);

        unsigned base = (unsigned)m.v.size();
        m.v.push_back({{a.x, roof.baseZ, a.y}, {roof.capColor.x, roof.capColor.y, roof.capColor.z, capAo},
                       {a.x * roof.uvScale, a.y * roof.uvScale}});
        m.v.push_back({{b.x, roof.baseZ, b.y}, {roof.capColor.x, roof.capColor.y, roof.capColor.z, capAo},
                       {b.x * roof.uvScale, b.y * roof.uvScale}});
        m.v.push_back({{b.x, roof.baseZ + hb, b.y}, {roof.capColor.x, roof.capColor.y, roof.capColor.z, capAo},
                       {b.x * roof.uvScale, b.y * roof.uvScale}});
        m.v.push_back({{a.x, roof.baseZ + ha, a.y}, {roof.capColor.x, roof.capColor.y, roof.capColor.z, capAo},
                       {a.x * roof.uvScale, a.y * roof.uvScale}});
        AddQuad(m, base + 0, base + 1, base + 2, base + 3);
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
