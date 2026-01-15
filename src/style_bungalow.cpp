#include "style_bungalow.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "building_compiler.h"
#include "building_model.h"
#include "geom_ops.h"
#include "geometry.h"

namespace {

Polygon2D BuildBaseFootprint(const sbl::BuildingPlan& plan)
{
    Polygon2D baseRect = PlaceRectInLot(plan.lot, plan.lotShrink, plan.lotSnap, plan.lotBiasDir, plan.lotBias);
    if(plan.forceFootprintAspect && plan.footprintAspect > 1.01f){
        float minShortHalf = std::max(2.0f, plan.lotSnap * 2.0f);
        baseRect = EnforceRectAspect(baseRect, plan.footprintAspect, minShortHalf);
    }
    if(plan.useFootprintSize){
        baseRect = FitRectToSize(baseRect, plan.footprintWidth, plan.footprintDepth);
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

Polygon2D BuildPorchFootprint(const Polygon2D& base, Vec2 biasDir)
{
    OBB2D obb = ComputeOBB(base);
    Vec2 axisX = obb.axisX;
    Vec2 axisY = obb.axisY;
    float biasLen = Length(biasDir);
    Vec2 front = (biasLen > 1e-3f) ? Normalize(biasDir) : axisY;
    if(Dot(front, axisY) < 0.0f){
        axisY = axisY * -1.0f;
    }
    float baseWidth = obb.halfX * 2.0f;
    float baseDepth = obb.halfY * 2.0f;
    float porchWidth = std::max(2.5f, baseWidth * 0.55f);
    porchWidth = std::min(porchWidth, baseWidth * 0.85f);
    float porchDepth = std::max(1.6f, baseDepth * 0.35f);
    porchDepth = std::min(porchDepth, baseDepth * 0.8f);
    Vec2 center = obb.center + axisY * (obb.halfY - porchDepth * 0.5f);
    float angle = std::atan2(axisX.y, axisX.x);
    return MakeRectangle(center, porchWidth, porchDepth, angle);
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
    Vec3 wallColor = PastelFromPoint(centroid);
    float wallHeight = std::max(plan.floorH, plan.totalFloors * plan.floorH);
    AddSlabVolume(model, base, 0.0f, wallHeight, wallColor, 0.02f, SlabRole::Residential);

    Polygon2D porch = BuildPorchFootprint(base, plan.lotBiasDir);
    AddSlabVolume(model, porch, 0.0f, plan.slabT * 0.6f, wallColor, 0.04f, SlabRole::Public);
    {
        OBB2D pobb = ComputeOBB(porch);
        Vec2 axisX = pobb.axisX;
        Vec2 axisY = pobb.axisY;
        float porchWidth = pobb.halfX * 2.0f;
        float porchDepth = pobb.halfY * 2.0f;
        float colInset = std::min(0.8f, porchWidth * 0.2f);
        float colHalf = 0.25f;
        float usableW = std::max(0.0f, porchWidth - colInset * 2.0f);
        int colCount = std::max(2, (int)std::floor(usableW / 4.0f) + 1);
        float spacing = (colCount > 1) ? (usableW / (colCount - 1)) : 0.0f;
        float frontOffset = pobb.halfY - std::min(0.5f, porchDepth * 0.35f);
        float colHeight = wallHeight * 0.85f;
        Vec2 baseCenter = pobb.center + axisY * frontOffset;
        for(int i=0;i<colCount;i++){
            float offset = -usableW * 0.5f + i * spacing;
            Vec2 c = baseCenter + axisX * offset;
            AddBoxVolume(model, c, axisX, axisY, colHalf, colHalf, 0.0f, colHeight, wallColor);
        }
    }

    RoofParams roof{};
    roof.type = (Hash01(seed ^ 0x9e3779b9U) < 0.35f) ? RoofType::Hip : RoofType::Gable;
    roof.pitchDeg = 22.0f + 12.0f * Hash01(seed ^ 0x7f4a7c15U);
    roof.overhang = 0.45f + 0.30f * Hash01(seed ^ 0x2b14e99fU);
    roof.ridgeHeight = plan.roofHeightOverride > 0.0f
                       ? plan.roofHeightOverride
                       : std::max(1.4f, plan.floorH * 0.55f);
    roof.hipRidgeFrac = 0.35f + 0.30f * Hash01(seed ^ 0x1b873593U);
    roof.aoStrength = 0.85f;
    Vec3 roofColor{0.28f, 0.18f, 0.12f};
    AddRoofVolume(model, base, base, wallHeight, roof, roofColor, wallColor, 0.05f);

    return CompileBuildingModel(model);
}
