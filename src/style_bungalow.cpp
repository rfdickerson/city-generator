#include "style_bungalow.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include "building_compiler.h"
#include "building_model.h"
#include "geom_ops.h"
#include "geometry.h"

namespace {

uint32_t SeedFromPoint(Vec2 p, uint32_t salt);
float Hash01(uint32_t x);

Polygon2D BuildBaseFootprint(const sbl::BuildingPlan& plan)
{
    Polygon2D baseRect = PlaceRectInLot(plan.lot, plan.lotShrink, plan.lotSnap, plan.lotBiasDir, plan.lotBias);
    if(plan.forceFootprintAspect && plan.footprintAspect > 1.01f){
        float minShortHalf = std::max(2.0f, plan.lotSnap * 2.0f);
        baseRect = EnforceRectAspect(baseRect, plan.footprintAspect, minShortHalf);
    }
    if(plan.useFootprintSize && plan.footprintWidth > 0.0f && plan.footprintDepth > 0.0f){
        baseRect = FitRectToSize(baseRect, plan.footprintWidth, plan.footprintDepth);
    }else if(plan.useFootprintSize){
        OBB2D obb = ComputeOBB(baseRect);
        Vec2 center = obb.center;
        uint32_t seed = SeedFromPoint(center, 0x3c6ef372U);
        float targetWidth = 28.0f + 6.0f * Hash01(seed ^ 0x1f123bb5U);
        float targetDepth = 32.0f + 8.0f * Hash01(seed ^ 0x9e3779b9U);
        float maxWidth = obb.halfX * 2.0f;
        float maxDepth = obb.halfY * 2.0f;
        targetWidth = std::min(targetWidth, maxWidth);
        targetDepth = std::min(targetDepth, maxDepth);

        float longEdge = std::max(targetWidth, targetDepth);
        float shortEdge = std::min(targetWidth, targetDepth);
        longEdge = std::min(longEdge, shortEdge * 1.5f);
        shortEdge = std::min(shortEdge, longEdge / 1.2f);

        float angle = std::atan2(obb.axisX.y, obb.axisX.x);
        if(Length(plan.lotBiasDir) > 1e-3f){
            Vec2 front = Normalize(plan.lotBiasDir);
            Vec2 streetAxis = Perp(front);
            angle = std::atan2(streetAxis.y, streetAxis.x);
        }
        baseRect = MakeRectangle(center, longEdge, shortEdge, angle);
    }
    if(!plan.useFootprintSize){
        OBB2D obb = ComputeOBB(baseRect);
        float maxInset = std::min(obb.halfX, obb.halfY) - 1.5f;
        if(maxInset > 0.25f){
            float extraInset = std::min(3.0f, maxInset);
            baseRect = baseRect.Inset(extraInset);
        }
    }
    return baseRect;
}

float Clamp(float v, float lo, float hi)
{
    return std::max(lo, std::min(hi, v));
}

float Clamp01(float v)
{
    return std::max(0.0f, std::min(1.0f, v));
}

uint32_t Hash32(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

float Hash01(uint32_t x)
{
    return (Hash32(x) & 0x00ffffff) / 16777215.0f;
}

Vec3 Mix(Vec3 a, Vec3 b, float t)
{
    return {a.x + (b.x - a.x) * t,
            a.y + (b.y - a.y) * t,
            a.z + (b.z - a.z) * t};
}

Vec3 Scale(Vec3 c, float s)
{
    return {Clamp01(c.x * s), Clamp01(c.y * s), Clamp01(c.z * s)};
}

uint32_t SeedFromPoint(Vec2 p, uint32_t salt)
{
    uint32_t sx = (uint32_t)std::floor((p.x + 1000.0f) * 10.0f);
    uint32_t sy = (uint32_t)std::floor((p.y + 1000.0f) * 10.0f);
    return sx ^ (sy * 0x9e3779b9U) ^ salt;
}

Vec3 PastelFromPoint(Vec2 p)
{
    uint32_t seed = SeedFromPoint(p, 0u);
    float r = 0.60f + 0.30f * Hash01(seed);
    float g = 0.60f + 0.30f * Hash01(seed ^ 0xa2c2f7b1U);
    float b = 0.60f + 0.30f * Hash01(seed ^ 0x85ebca6bU);
    return {Clamp01(r), Clamp01(g), Clamp01(b)};
}

Vec3 HsvToRgb(float h, float s, float v)
{
    float c = v * s;
    float h6 = std::fmod(h * 6.0f, 6.0f);
    float x = c * (1.0f - std::fabs(std::fmod(h6, 2.0f) - 1.0f));
    float m = v - c;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    if(h6 < 1.0f){
        r = c; g = x; b = 0.0f;
    }else if(h6 < 2.0f){
        r = x; g = c; b = 0.0f;
    }else if(h6 < 3.0f){
        r = 0.0f; g = c; b = x;
    }else if(h6 < 4.0f){
        r = 0.0f; g = x; b = c;
    }else if(h6 < 5.0f){
        r = x; g = 0.0f; b = c;
    }else{
        r = c; g = 0.0f; b = x;
    }
    return {Clamp01(r + m), Clamp01(g + m), Clamp01(b + m)};
}

Vec3 SaturatedPastelFromPoint(Vec2 p)
{
    uint32_t seed = SeedFromPoint(p, 0u);
    float hue = Hash01(seed ^ 0x6a09e667U);
    float sat = 0.55f + 0.20f * Hash01(seed ^ 0xbb67ae85U);
    float val = 0.88f + 0.10f * Hash01(seed ^ 0x3c6ef372U);
    return HsvToRgb(hue, sat, val);
}

float MinEdgeLength(const Polygon2D& poly)
{
    float minEdge = std::numeric_limits<float>::max();
    int n = (int)poly.v.size();
    for(int i = 0; i < n; ++i){
        Vec2 a = poly.v[i];
        Vec2 b = poly.v[(i + 1) % n];
        minEdge = std::min(minEdge, Length({b.x - a.x, b.y - a.y}));
    }
    return minEdge;
}

Polygon2D InsetIfUsable(const Polygon2D& fp, float inset, float minAreaRatio)
{
    if(inset <= 1e-4f){
        return fp;
    }
    Polygon2D insetFp = fp.Inset(inset);
    float areaBase = std::fabs(SignedArea(fp.v));
    float areaInset = std::fabs(SignedArea(insetFp.v));
    if(areaBase <= 1e-4f || areaInset < areaBase * minAreaRatio){
        return fp;
    }
    return insetFp;
}

float MaxProjection(const Polygon2D& poly, Vec2 dir)
{
    float maxProj = -std::numeric_limits<float>::max();
    for(const auto& p : poly.v){
        maxProj = std::max(maxProj, Dot(p, dir));
    }
    return maxProj;
}

struct RoofFrame2D {
    Vec2 center;
    Vec2 axisU;
    Vec2 axisV;
    float halfU;
    float halfV;
};

RoofFrame2D ComputeRoofFrame(const Polygon2D& footprint)
{
    OBB2D obb = ComputeOBB(footprint);
    bool useX = obb.halfX >= obb.halfY;
    Vec2 axisU = useX ? obb.axisX : obb.axisY;
    axisU = Normalize(axisU);
    Vec2 axisV = Normalize(Perp(axisU));
    float halfU = useX ? obb.halfX : obb.halfY;
    float halfV = useX ? obb.halfY : obb.halfX;
    return {obb.center, axisU, axisV, halfU, halfV};
}

Polygon2D InsertRidgeVerts(const Polygon2D& footprint, const RoofFrame2D& frame)
{
    std::vector<Vec2> out;
    int n = (int)footprint.v.size();
    if(n < 3){
        return footprint;
    }
    const float eps = 1e-4f;
    for(int i = 0; i < n; ++i){
        Vec2 a = footprint.v[i];
        Vec2 b = footprint.v[(i + 1) % n];
        out.push_back(a);
        float va = Dot(a - frame.center, frame.axisV);
        float vb = Dot(b - frame.center, frame.axisV);
        if((va > eps && vb < -eps) || (va < -eps && vb > eps)){
            float t = va / (va - vb);
            if(t > eps && t < 1.0f - eps){
                Vec2 hit{a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
                out.push_back(hit);
            }
        }
    }
    if(out.size() < 3){
        return footprint;
    }
    return {out};
}

void AddWallQuad(Mesh& m,
                 Vec2 center,
                 Vec2 axisLong,
                 Vec2 axisShort,
                 float halfW,
                 float z0,
                 float z1,
                 float outward,
                 Vec3 color,
                 float ao,
                 float uvScale)
{
    Vec2 offset = axisShort * outward;
    Vec2 left = center - axisLong * halfW + offset;
    Vec2 right = center + axisLong * halfW + offset;
    float u0 = Dot(left, axisLong) * uvScale;
    float u1 = Dot(right, axisLong) * uvScale;

    unsigned base = (unsigned)m.v.size();
    m.v.push_back({{left.x, z0, left.y}, {color.x, color.y, color.z, ao}, {u0, z0 * uvScale}});
    m.v.push_back({{right.x, z0, right.y}, {color.x, color.y, color.z, ao}, {u1, z0 * uvScale}});
    m.v.push_back({{right.x, z1, right.y}, {color.x, color.y, color.z, ao}, {u1, z1 * uvScale}});
    m.v.push_back({{left.x, z1, left.y}, {color.x, color.y, color.z, ao}, {u0, z1 * uvScale}});
    AddQuad(m, base + 0, base + 1, base + 2, base + 3);
}

} // namespace

Mesh BuildBungalowBuilding(const sbl::BuildingPlan& plan)
{
    BuildingModel model;

    if(plan.showLot){
        AddSlabVolume(model, plan.lot, -0.25f, 0.25f, plan.lotFill, 0.03f, SlabRole::Public);
    }

    Polygon2D base = BuildBaseFootprint(plan);
    Vec2 centroid = Centroid(base);
    uint32_t seed = SeedFromPoint(centroid, 0x3b9aca00U);
    Vec3 wallColor = SaturatedPastelFromPoint(centroid);
    Vec3 foundationColor = Mix(plan.concrete, wallColor, 0.35f);
    Vec3 trimColor = Mix(wallColor, {1.0f, 1.0f, 1.0f}, 0.12f);
    Vec3 porchColor = Mix(foundationColor, wallColor, 0.20f);
    Vec3 columnColor = Mix(trimColor, foundationColor, 0.25f);
    float wallHeight = std::max(8.0f, std::min(9.0f, plan.floorH));
    float foundationHeight = std::min(wallHeight * 0.25f, std::max(0.6f, plan.slabT * 1.25f));

    float minEdge = MinEdgeLength(base);
    float wallInset = std::max(plan.lotSnap * 2.5f, 0.8f);
    wallInset = std::min(wallInset, minEdge * 0.2f);
    Polygon2D body = InsetIfUsable(base, wallInset, 0.6f);

    AddSlabVolume(model, base, 0.0f, foundationHeight, foundationColor, 0.03f, SlabRole::Public);
    AddSlabVolume(model, body, foundationHeight, wallHeight - foundationHeight, wallColor, 0.02f, SlabRole::Residential);

    RoofParams roof{};
    roof.type = RoofType::Gable;
    float basePitch = 22.0f + 6.0f * Hash01(seed ^ 0x7f4a7c15U);
    roof.pitchDeg = basePitch;
    roof.overhang = 0.75f + 0.20f * Hash01(seed ^ 0x2b14e99fU);
    bool hasRoofOverride = plan.roofHeightOverride > 0.0f;
    float ridgeHeight = hasRoofOverride ? plan.roofHeightOverride
                                        : (5.0f + 2.5f * Hash01(seed ^ 0x1b873593U));
    RoofFrame2D frame = ComputeRoofFrame(body);
    float slope = std::tan(roof.pitchDeg * 3.14159265f / 180.0f);
    if(!hasRoofOverride){
        float minRidge = slope * frame.halfV * 0.85f;
        ridgeHeight = std::max(ridgeHeight, minRidge);
    }
    float maxRidge = slope * frame.halfV * 1.05f;
    ridgeHeight = std::min(ridgeHeight, maxRidge);
    roof.ridgeHeight = std::max(2.0f, ridgeHeight);
    roof.hipRidgeFrac = 0.40f;
    roof.aoStrength = 0.85f;
    float roofTint = 0.90f + 0.10f * Hash01(seed ^ 0x9e3779b9U);
    Vec3 roofColor = Scale({0.28f, 0.18f, 0.12f}, roofTint);

    OBB2D obb = ComputeOBB(base);
    bool useX = obb.halfX >= obb.halfY;
    Vec2 axisLong = Normalize(useX ? obb.axisX : obb.axisY);
    Vec2 axisShort = Normalize(useX ? obb.axisY : obb.axisX);
    float halfLong = useX ? obb.halfX : obb.halfY;
    float halfShort = useX ? obb.halfY : obb.halfX;
    if(Length(plan.lotBiasDir) > 1e-3f){
        Vec2 front = Normalize(plan.lotBiasDir);
        if(Dot(front, axisShort) < 0.0f){
            axisShort = axisShort * -1.0f;
        }
    }
    float baseWidth = halfLong * 2.0f;
    float baseDepth = halfShort * 2.0f;
    float porchWidth = Clamp(baseWidth * 0.65f, 3.0f, baseWidth * 0.92f);
    float porchDepth = Clamp(baseDepth * 0.32f, 2.2f, baseDepth * 0.48f);
    float desiredOutset = std::min(1.4f, baseDepth * 0.12f);
    float lotFront = MaxProjection(plan.lot, axisShort);
    float baseCenterProj = Dot(obb.center, axisShort);
    float baseFront = baseCenterProj + halfShort;
    float frontMargin = lotFront - baseFront;
    float maxOutset = std::max(0.0f, frontMargin - plan.lotSnap);
    maxOutset = std::min(maxOutset, roof.overhang * 0.85f);
    float porchOutset = std::min(desiredOutset, maxOutset);
    float minFront = baseFront - porchDepth * 0.15f;
    float maxFront = std::max(minFront, lotFront - plan.lotSnap);
    float targetFront = std::min(baseFront + porchOutset, maxFront);
    float porchCenterProj = targetFront - porchDepth * 0.5f;
    float porchAngle = std::atan2(axisLong.y, axisLong.x);
    Vec2 porchCenter = obb.center + axisShort * (porchCenterProj - baseCenterProj);
    Polygon2D porch = MakeRectangle(porchCenter, porchWidth, porchDepth, porchAngle);
    Polygon2D roofFootprint = UnionConvexHull(body, porch);

    float deckThickness = std::max(0.18f, plan.slabT * 0.45f);
    // Nudge the deck to avoid z-fighting with the foundation top.
    float deckTop = foundationHeight + 0.02f;
    float deckZ0 = std::max(0.0f, deckTop - deckThickness);
    AddSlabVolume(model, porch, deckZ0, deckThickness, porchColor, 0.04f, SlabRole::Public);

    float colTopZ = wallHeight - 0.4f;
    float colBaseZ = deckTop;
    if(colTopZ <= colBaseZ + 0.6f){
        colTopZ = colBaseZ + 0.6f;
    }
    float colHeight = colTopZ - colBaseZ;
    float colHalf = Clamp(porchWidth * 0.035f, 0.22f, 0.35f);
    float colBaseHalf = colHalf * 1.25f;
    float colInset = std::min(0.9f, porchWidth * 0.18f);
    float usableW = std::max(0.0f, porchWidth - colInset * 2.0f);
    int colCount = std::max(2, (int)std::floor(usableW / 4.5f) + 1);
    float spacing = (colCount > 1) ? (usableW / (colCount - 1)) : 0.0f;
    float frontInset = std::min(0.35f, porchDepth * 0.2f);
    float porchFront = porchCenterProj + porchDepth * 0.5f;
    Vec2 columnRow = obb.center + axisShort * (porchFront - frontInset - baseCenterProj);
    float plinthH = std::min(0.5f, colHeight * 0.18f);
    int placedColumns = 0;
    for(int i = 0; i < colCount; ++i){
        float offset = -usableW * 0.5f + i * spacing;
        Vec2 c = columnRow + axisLong * offset;
        if(PointInPolygon(body, c)){
            continue;
        }
        AddBoxVolume(model, c, axisLong, axisShort, colBaseHalf, colBaseHalf, colBaseZ, colBaseZ + plinthH, foundationColor);
        AddBoxVolume(model, c, axisLong, axisShort, colHalf, colHalf, colBaseZ + plinthH, colTopZ, columnColor);
        placedColumns += 1;
    }
    if(placedColumns >= 2){
        float beamH = std::min(0.28f, colHeight * 0.12f);
        float beamDepth = std::min(0.4f, porchDepth * 0.25f);
        float beamHalfX = usableW * 0.5f + colHalf;
        Vec2 beamCenter = porchCenter + axisShort * (porchDepth * 0.5f - frontInset * 0.5f);
        AddBoxVolume(model, beamCenter, axisLong, axisShort, beamHalfX, beamDepth * 0.5f,
                     colTopZ - beamH, colTopZ, trimColor);
    }

    Mesh wallDetails;
    Vec3 windowColor{0.55f, 0.78f, 0.98f};
    OBB2D bodyObb = ComputeOBB(body);
    bool bodyUseX = bodyObb.halfX >= bodyObb.halfY;
    Vec2 bodyAxisLong = Normalize(bodyUseX ? bodyObb.axisX : bodyObb.axisY);
    Vec2 bodyAxisShort = Normalize(bodyUseX ? bodyObb.axisY : bodyObb.axisX);
    float bodyHalfLong = bodyUseX ? bodyObb.halfX : bodyObb.halfY;
    float bodyHalfShort = bodyUseX ? bodyObb.halfY : bodyObb.halfX;
    float panelOffset = 0.03f;
    float bandZ0 = foundationHeight + 0.25f;
    float bandZ1 = bandZ0 + 0.7f;
    float bandInset = std::min(1.2f, bodyHalfLong * 0.1f);
    float bandHalfW = std::max(0.6f, bodyHalfLong - bandInset);
    for(int side = -1; side <= 1; side += 2){
        Vec2 bandCenter = bodyObb.center + bodyAxisShort * ((bodyHalfShort + panelOffset) * (float)side);
        AddWallQuad(wallDetails, bandCenter, bodyAxisLong, bodyAxisShort, bandHalfW,
                    bandZ0, bandZ1, 0.0f, trimColor, 0.88f, 0.1f);
    }

    float windowHalfW = Clamp(bodyHalfLong * 0.12f, 0.7f, 1.4f);
    float windowHalfH = 0.8f;
    float windowZ0 = foundationHeight + std::min(2.3f, wallHeight * 0.35f);
    float windowZ1 = windowZ0 + windowHalfH * 2.0f;
    float longInset = std::max(1.2f, windowHalfW * 1.5f);
    float usableLong = std::max(0.0f, bodyHalfLong * 2.0f - longInset * 2.0f);
    int longCount = std::max(2, (int)std::floor(usableLong / 5.5f) + 1);
    longCount = std::min(longCount, 4);
    float longSpacing = (longCount > 1) ? (usableLong / (longCount - 1)) : 0.0f;
    float sideOffset = bodyHalfShort + panelOffset;
    for(int side = -1; side <= 1; side += 2){
        Vec2 sideCenter = bodyObb.center + bodyAxisShort * (sideOffset * (float)side);
        for(int i = 0; i < longCount; ++i){
            float offset = -usableLong * 0.5f + i * longSpacing;
            Vec2 c = sideCenter + bodyAxisLong * offset;
            AddWallQuad(wallDetails, c, bodyAxisLong, bodyAxisShort, windowHalfW,
                        windowZ0, windowZ1, 0.0f, windowColor, 0.92f, 0.1f);
        }
    }

    // Insert ridge vertices so gable end caps follow the roof slope.
    Polygon2D gableCap = InsertRidgeVerts(body, frame);
    AddRoofVolume(model, roofFootprint, gableCap, wallHeight, roof, roofColor, trimColor, 0.05f);

    Mesh building = CompileBuildingModel(model);
    Append(building, wallDetails);
    return building;
}
