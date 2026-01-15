#include "style_brutalist.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "building_compiler.h"
#include "building_model.h"
#include "geom_ops.h"
#include "geometry.h"

namespace {

struct Footprints {
    Polygon2D baseRect;
    Polygon2D base;
    Polygon2D towerBase;
};

SlabRole ToMeshRole(sbl::SlabRole role)
{
    switch(role){
    case sbl::SlabRole::Infrastructure:
        return SlabRole::Infrastructure;
    case sbl::SlabRole::Podium:
        return SlabRole::Podium;
    case sbl::SlabRole::Public:
        return SlabRole::Public;
    case sbl::SlabRole::Residential:
        return SlabRole::Residential;
    case sbl::SlabRole::Terrace:
        return SlabRole::Terrace;
    case sbl::SlabRole::Mechanical:
        return SlabRole::Mechanical;
    case sbl::SlabRole::Roof:
        return SlabRole::Roof;
    case sbl::SlabRole::Office:
    default:
        return SlabRole::Office;
    }
}

sbl::SlabRole RoleForFloor(const sbl::BuildingPlan& plan, int floor)
{
    for(const auto& slab : plan.slabPlan){
        if(floor >= slab.startFloor && floor < slab.startFloor + slab.floorCount){
            return slab.role;
        }
    }
    return sbl::SlabRole::Office;
}

Polygon2D InsetIfUsable(const Polygon2D& fp, float inset)
{
    if(inset <= 1e-4f){
        return fp;
    }
    Polygon2D insetFp = fp.Inset(inset);
    float areaBase = std::fabs(SignedArea(fp.v));
    float areaInset = std::fabs(SignedArea(insetFp.v));
    if(areaBase <= 1e-4f || areaInset < areaBase * 0.45f){
        return fp;
    }
    return insetFp;
}

float PilotisGroundInset(const Polygon2D& fp, const sbl::BuildingPlan& plan)
{
    float minEdge = std::numeric_limits<float>::max();
    int n = (int)fp.v.size();
    for(int i=0;i<n;i++){
        Vec2 a = fp.v[i];
        Vec2 b = fp.v[(i+1)%n];
        minEdge = std::min(minEdge, Length({b.x-a.x, b.y-a.y}));
    }
    float inset = std::max(plan.lotSnap * 1.5f, 1.0f);
    inset = std::min(inset, minEdge * 0.25f);
    return inset;
}

void AddPilotisCore(BuildingModel& model, const Polygon2D& base, const sbl::BuildingPlan& plan, float pilotisHeight)
{
    if(pilotisHeight <= 0.0f){
        return;
    }
    float inset = PilotisGroundInset(base, plan);
    Polygon2D coreFp = InsetIfUsable(base, inset);
    AddSlabVolume(model, coreFp, 0.0f, pilotisHeight, plan.concrete, 0.02f, SlabRole::Public);
}

Footprints ComputeFootprints(const sbl::BuildingPlan& plan)
{
    Footprints f;
    f.baseRect = PlaceRectInLot(plan.lot, plan.lotShrink, plan.lotSnap, plan.lotBiasDir, plan.lotBias);
    if(plan.forceFootprintAspect && plan.footprintAspect > 1.01f){
        float minShortHalf = std::max(2.0f, plan.lotSnap * 2.0f);
        f.baseRect = EnforceRectAspect(f.baseRect, plan.footprintAspect, minShortHalf);
    }
    if(plan.useFootprintSize){
        f.baseRect = FitRectToSize(f.baseRect, plan.footprintWidth, plan.footprintDepth);
    }
    f.base = f.baseRect;
    if(plan.useLShape){
        f.base = MakeLShapeFootprint(f.baseRect, plan.lCutX, plan.lCutY);
    }
    f.towerBase = f.base;
    if(plan.usePodiumTower && !plan.useLShape && plan.podiumFloors < plan.totalFloors){
        f.towerBase = f.base.Inset(plan.towerInset);
    }
    return f;
}

float ComputePilotisHeight(const sbl::BuildingPlan& plan)
{
    return plan.enablePilotis ? plan.pilotisHeight : 0.0f;
}

void AddLotMesh(BuildingModel& model, const sbl::BuildingPlan& plan)
{
    if(!plan.showLot){
        return;
    }
    AddSlabVolume(model, plan.lot, -0.25f, 0.25f, plan.lotFill, 0.03f, SlabRole::Public);
}

void AddFloorSlab(BuildingModel& model, const Polygon2D& fp, float z, const sbl::BuildingPlan& plan, int floor)
{
    SlabRole role = ToMeshRole(RoleForFloor(plan, floor));
    AddSlabVolume(model, fp, z, plan.slabT, plan.concrete, 0.02f, role);
}

void AddPodiumRoof(BuildingModel& model, const Polygon2D& base, float pilotisHeight, const sbl::BuildingPlan& plan)
{
    if(!plan.usePodiumTower || plan.useLShape || plan.podiumFloors <= 0 || plan.podiumFloors >= plan.totalFloors){
        return;
    }
    float podiumZ = pilotisHeight + plan.podiumFloors * plan.floorH - 0.02f;
    AddSlabVolume(model, base, podiumZ, 0.35f, plan.concrete, 0.02f, SlabRole::Podium);
}

void AddPilotis(BuildingModel& model, const Polygon2D& baseRect, const Polygon2D& base, const sbl::BuildingPlan& plan, float pilotisHeight)
{
    if(pilotisHeight <= 0.0f){
        return;
    }
    Vec2 center = Centroid(baseRect);
    Vec2 axisX = Normalize({baseRect.v[0].x-baseRect.v[1].x, baseRect.v[0].y-baseRect.v[1].y});
    Vec2 axisY = Normalize({baseRect.v[1].x-baseRect.v[2].x, baseRect.v[1].y-baseRect.v[2].y});

    float halfX = 0.5f * Length({baseRect.v[0].x-baseRect.v[1].x, baseRect.v[0].y-baseRect.v[1].y});
    float halfY = 0.5f * Length({baseRect.v[1].x-baseRect.v[2].x, baseRect.v[1].y-baseRect.v[2].y});

    float edgeInset = 2.0f;
    float spacing = 4.0f;
    float colHalf = 0.35f;

    float usableX = std::max(0.0f, halfX - edgeInset);
    float usableY = std::max(0.0f, halfY - edgeInset);

    for(float x=-usableX; x<=usableX+0.01f; x+=spacing){
        for(float y=-usableY; y<=usableY+0.01f; y+=spacing){
            Vec2 c = center + axisX * x + axisY * y;
            if(!PointInPolygon(base, c)){
                continue;
            }
            AddBoxVolume(model, c, axisX, axisY, colHalf, colHalf, 0.0f, pilotisHeight, plan.concrete);
        }
    }
}

void AddRoofCap(BuildingModel& model, const Polygon2D& capBase, float totalHeight, const sbl::BuildingPlan& plan)
{
    float capT = std::max(plan.roofCapT, plan.slabT * 1.25f);
    Polygon2D capFp = plan.useLShape ? capBase : OutsetFromCentroid(capBase, plan.roofCapOverhang);
    float capZ = totalHeight - capT;
    AddSlabVolume(model, capFp, capZ, capT, plan.concrete, 0.02f, SlabRole::Roof);
}

void AddRoofDeck(BuildingModel& model, const Polygon2D& deckBase, float totalHeight, const sbl::BuildingPlan& plan)
{
    Polygon2D deckFp = plan.useLShape ? deckBase : OutsetFromCentroid(deckBase, -plan.roofDeckInset);
    float deckZ = totalHeight + 0.02f;
    AddSlabVolume(model, deckFp, deckZ, plan.roofDeckT, plan.roofDeck, 0.02f, SlabRole::Terrace);
    if(plan.roofDeckEnclosed){
        float wallZ0 = deckZ + plan.roofDeckT;
        float wallH = 1.1f;
        AddCurtainWallVolume(model,
                             deckFp,
                             wallZ0,
                             wallZ0 + wallH,
                             0.0f,
                             plan.concrete,
                             plan.concrete,
                             50.0f,
                             0.0f,
                             0.3f);
    }
}

} // namespace

Mesh BuildBrutalistBuilding(const sbl::BuildingPlan& plan)
{
    Footprints fp = ComputeFootprints(plan);
    float pilotisHeight = ComputePilotisHeight(plan);
    float totalHeight = pilotisHeight + plan.totalFloors * plan.floorH;

    BuildingModel model;
    AddLotMesh(model, plan);
    AddPilotisCore(model, fp.base, plan, pilotisHeight);

    for(int f=0;f<plan.totalFloors;f++){
        Polygon2D floorFp = fp.base;
        if(plan.usePodiumTower && !plan.useLShape && f >= plan.podiumFloors){
            floorFp = fp.towerBase;
        }
        float z = pilotisHeight + f * plan.floorH;
        AddFloorSlab(model, floorFp, z, plan, f);
    }

    AddPodiumRoof(model, fp.base, pilotisHeight, plan);
    AddPilotis(model, fp.baseRect, fp.base, plan, pilotisHeight);

    Polygon2D capBase = fp.base;
    if(plan.usePodiumTower && !plan.useLShape && plan.podiumFloors < plan.totalFloors){
        capBase = fp.towerBase;
    }
    AddRoofCap(model, capBase, totalHeight, plan);
    AddRoofDeck(model, capBase, totalHeight, plan);

    return CompileBuildingModel(model);
}
