#include "style_brutalist.h"

#include <algorithm>

#include "geom_ops.h"
#include "geometry.h"
#include "mesh.h"

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

Footprints ComputeFootprints(const sbl::BuildingPlan& plan)
{
    Footprints f;
    f.baseRect = PlaceRectInLot(plan.lot, plan.lotShrink, plan.lotSnap, plan.lotBiasDir, plan.lotBias);
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

void AddLotMesh(Mesh& out, const sbl::BuildingPlan& plan)
{
    if(!plan.showLot){
        return;
    }
    Mesh lotMesh = BuildSlab({plan.lot,-0.25f,0.25f},plan.lotFill,0.03f, SlabRole::Public);
    Append(out, lotMesh);
}

void AddFloorSlab(Mesh& out, const Polygon2D& fp, float z, const sbl::BuildingPlan& plan, int floor)
{
    SlabRole role = ToMeshRole(RoleForFloor(plan, floor));
    Mesh slab = BuildSlab({fp,z,plan.slabT},plan.concrete,0.02f, role);
    Append(out, slab);
}

void AddPodiumRoof(Mesh& out, const Polygon2D& base, float pilotisHeight, const sbl::BuildingPlan& plan)
{
    if(!plan.usePodiumTower || plan.useLShape || plan.podiumFloors <= 0 || plan.podiumFloors >= plan.totalFloors){
        return;
    }
    float podiumZ = pilotisHeight + plan.podiumFloors * plan.floorH - 0.02f;
    Mesh podiumRoof = BuildSlab({base,podiumZ,0.35f},plan.concrete,0.02f, SlabRole::Podium);
    Append(out, podiumRoof);
}

void AddPilotis(Mesh& out, const Polygon2D& baseRect, const Polygon2D& base, const sbl::BuildingPlan& plan, float pilotisHeight)
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
            AddBox(out, c, axisX, axisY, colHalf, colHalf, 0.0f, pilotisHeight, plan.concrete);
        }
    }
}

void AddRoofCap(Mesh& out, const Polygon2D& capBase, float totalHeight, const sbl::BuildingPlan& plan)
{
    float capT = std::max(plan.roofCapT, plan.slabT * 1.25f);
    Polygon2D capFp = plan.useLShape ? capBase : OutsetFromCentroid(capBase, plan.roofCapOverhang);
    float capZ = totalHeight - capT;
    Mesh cap = BuildSlab({capFp,capZ,capT},plan.concrete,0.02f, SlabRole::Roof);
    Append(out, cap);
}

} // namespace

Mesh BuildBrutalistBuilding(const sbl::BuildingPlan& plan)
{
    Footprints fp = ComputeFootprints(plan);
    float pilotisHeight = ComputePilotisHeight(plan);
    float totalHeight = pilotisHeight + plan.totalFloors * plan.floorH;

    Mesh building;
    AddLotMesh(building, plan);

    for(int f=0;f<plan.totalFloors;f++){
        Polygon2D floorFp = fp.base;
        if(plan.usePodiumTower && !plan.useLShape && f >= plan.podiumFloors){
            floorFp = fp.towerBase;
        }

        float z = pilotisHeight + f * plan.floorH;
        AddFloorSlab(building, floorFp, z, plan, f);
    }

    AddPodiumRoof(building, fp.base, pilotisHeight, plan);
    AddPilotis(building, fp.baseRect, fp.base, plan, pilotisHeight);

    Polygon2D capBase = fp.base;
    if(plan.usePodiumTower && !plan.useLShape && plan.podiumFloors < plan.totalFloors){
        capBase = fp.towerBase;
    }
    AddRoofCap(building, capBase, totalHeight, plan);

    return building;
}
