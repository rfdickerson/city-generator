#include "style_bungalow.h"

#include <algorithm>
#include <cmath>

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

Vec3 Darken(Vec3 c, float mul)
{
    return {c.x * mul, c.y * mul, c.z * mul};
}

} // namespace

Mesh BuildBungalowBuilding(const sbl::BuildingPlan& plan)
{
    BuildingModel model;

    if(plan.showLot){
        AddSlabVolume(model, plan.lot, -0.25f, 0.25f, plan.lotFill, 0.03f, SlabRole::Public);
    }

    Polygon2D base = BuildBaseFootprint(plan);
    float wallHeight = std::max(plan.floorH, plan.totalFloors * plan.floorH);
    AddSlabVolume(model, base, 0.0f, wallHeight, plan.concrete, 0.02f, SlabRole::Residential);

    Polygon2D porch = BuildPorchFootprint(base, plan.lotBiasDir);
    AddSlabVolume(model, porch, 0.0f, plan.slabT * 0.6f, plan.concrete, 0.04f, SlabRole::Public);

    RoofParams roof{};
    roof.type = RoofType::Gable;
    roof.pitchDeg = 28.0f;
    roof.overhang = 0.65f;
    roof.ridgeHeight = std::max(1.4f, plan.floorH * 0.55f);
    roof.hipRidgeFrac = 0.45f;
    roof.aoStrength = 0.85f;
    Vec3 roofColor = Darken(plan.roofDeck, 0.75f);
    AddRoofVolume(model, base, wallHeight, roof, roofColor, 0.05f);

    return CompileBuildingModel(model);
}
