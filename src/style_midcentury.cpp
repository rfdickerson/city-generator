#include "style_midcentury.h"

#include <algorithm>
#include <cmath>
#include <limits>

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

bool AllowsCurtain(const sbl::BuildingPlan& plan)
{
    if(plan.fenestration == sbl::FenestrationPattern::None){
        return false;
    }
    return plan.facadeType == sbl::FacadeType::CurtainWall ||
           plan.facadeType == sbl::FacadeType::BriseSoleil ||
           plan.facadeType == sbl::FacadeType::Screened;
}

bool AllowsBriseSoleil(const sbl::BuildingPlan& plan)
{
    return plan.facadeType == sbl::FacadeType::BriseSoleil;
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

void AddPilotisCore(Mesh& out, const Polygon2D& base, const sbl::BuildingPlan& plan, float pilotisHeight)
{
    if(pilotisHeight <= 0.0f){
        return;
    }
    float inset = PilotisGroundInset(base, plan);
    Polygon2D coreFp = InsetIfUsable(base, inset);
    Mesh core = BuildSlab({coreFp, 0.0f, pilotisHeight}, plan.concrete, 0.02f, SlabRole::Public);
    Append(out, core);
}

Footprints ComputeFootprints(const sbl::BuildingPlan& plan)
{
    Footprints f;
    f.baseRect = PlaceRectInLot(plan.lot, plan.lotShrink, plan.lotSnap, plan.lotBiasDir, plan.lotBias);
    if(plan.forceFootprintAspect && plan.footprintAspect > 1.01f){
        float minShortHalf = std::max(2.0f, plan.lotSnap * 2.0f);
        f.baseRect = EnforceRectAspect(f.baseRect, plan.footprintAspect, minShortHalf);
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

void AddLotMesh(Mesh& out, const sbl::BuildingPlan& plan)
{
    if(!plan.showLot){
        return;
    }
    Mesh lotMesh = BuildSlab({plan.lot,-0.25f,0.25f},plan.lotFill,0.03f, SlabRole::Public);
    Append(out, lotMesh);
}

void AddFloorSlab(Mesh& out, const Polygon2D& fp, float z, const sbl::BuildingPlan& plan, bool isTop, int floor)
{
    Vec3 slabColor = isTop ? plan.roofDeck : plan.concrete;
    SlabRole role = ToMeshRole(RoleForFloor(plan, floor));
    Mesh slab = BuildSlab({fp,z,plan.slabT},slabColor,0.02f, role);
    Append(out, slab);
}

bool AddCurtainWallBand(Mesh& out,
                        const Polygon2D& fp,
                        float z,
                        float bandTop,
                        float totalHeight,
                        const sbl::BuildingPlan& plan)
{
    float bandFloors = std::max(1, plan.curtainBandFloors);
    float cwTop = std::min(z + bandFloors * plan.floorH, bandTop);
    cwTop = std::min(cwTop, totalHeight - plan.roofCapT);
    if(cwTop <= z + plan.slabT){
        return false;
    }
    float minEdge = std::numeric_limits<float>::max();
    int n = (int)fp.v.size();
    for(int i=0;i<n;i++){
        Vec2 a = fp.v[i];
        Vec2 b = fp.v[(i+1)%n];
        minEdge = std::min(minEdge, Length({b.x-a.x, b.y-a.y}));
    }
    float inset = std::min(plan.curtainInset, minEdge * 0.35f);
    Polygon2D cwFp = fp.Inset(inset);
    float areaBase = std::fabs(SignedArea(fp.v));
    float areaCw = std::fabs(SignedArea(cwFp.v));
    if(areaBase <= 1e-4f || areaCw < areaBase * 0.15f){
        cwFp = fp;
    }
    Mesh cw = BuildCurtainWall(cwFp,
                               z+plan.slabT,
                               cwTop,
                               0.0f,
                               plan.window,
                               plan.concrete,
                               2.2f,
                               0.35f,
                               0.15f);
    Append(out, cw);
    return true;
}

void AddBriseSoleil(Mesh& out, const Polygon2D& fp, float z, int f, const sbl::BuildingPlan& plan)
{
    if(!AllowsBriseSoleil(plan)){
        return;
    }
    if(plan.finEvery <= 0 || (f + 1) % plan.finEvery != 0 || f == plan.totalFloors - 1){
        return;
    }
    float finZ = z + plan.floorH - plan.finThickness * 0.5f;
    Polygon2D finFp = OutsetFromCentroid(fp, plan.finProjection);
    Mesh fin = BuildSlab({finFp,finZ,plan.finThickness},plan.concrete,0.02f, SlabRole::Terrace);
    Append(out, fin);
}

void AddPodiumRoof(Mesh& out, const Polygon2D& base, float pilotisHeight, const sbl::BuildingPlan& plan)
{
    if(!plan.usePodiumTower || plan.useLShape || plan.podiumFloors <= 0 || plan.podiumFloors >= plan.totalFloors){
        return;
    }
    float podiumZ = pilotisHeight + plan.podiumFloors * plan.floorH - 0.02f;
    Mesh podiumRoof = BuildSlab({base,podiumZ,0.25f},plan.concrete,0.02f, SlabRole::Podium);
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
    float colHalf = 0.25f;

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
    Polygon2D capFp = plan.useLShape ? capBase : OutsetFromCentroid(capBase, plan.roofCapOverhang);
    float capZ = totalHeight - plan.roofCapT;
    Mesh cap = BuildSlab({capFp,capZ,plan.roofCapT},plan.concrete,0.02f, SlabRole::Roof);
    Append(out, cap);
}

void AddRoofDeck(Mesh& out, const Polygon2D& deckBase, float totalHeight, const sbl::BuildingPlan& plan)
{
    Polygon2D deckFp = plan.useLShape ? deckBase : OutsetFromCentroid(deckBase, -plan.roofDeckInset);
    float deckZ = totalHeight + 0.02f;
    Mesh deck = BuildSlab({deckFp,deckZ,plan.roofDeckT},plan.roofDeck,0.02f, SlabRole::Terrace);
    Append(out, deck);
    if(plan.roofDeckEnclosed){
        float wallZ0 = deckZ + plan.roofDeckT;
        float wallH = 1.1f;
        Mesh wall = BuildCurtainWall(deckFp,
                                     wallZ0,
                                     wallZ0 + wallH,
                                     0.0f,
                                     plan.concrete,
                                     plan.concrete,
                                     50.0f,
                                     0.0f,
                                     0.3f);
        Append(out, wall);
    }
}

} // namespace

Mesh BuildMidcenturyBuilding(const sbl::BuildingPlan& plan)
{
    Footprints fp = ComputeFootprints(plan);
    float pilotisHeight = ComputePilotisHeight(plan);
    float totalHeight = pilotisHeight + plan.totalFloors * plan.floorH;

    Mesh building;
    AddLotMesh(building, plan);
    AddPilotisCore(building, fp.base, plan, pilotisHeight);

    bool addedCurtain = false;
    for(int f=0;f<plan.totalFloors;f++){
        Polygon2D floorFp = fp.base;
        if(plan.usePodiumTower && !plan.useLShape && f >= plan.podiumFloors){
            floorFp = fp.towerBase;
        }
        float z = pilotisHeight + f * plan.floorH;
        AddFloorSlab(building, floorFp, z, plan, f == plan.totalFloors - 1, f);

        if(AllowsCurtain(plan) && plan.curtainEvery > 0 && f % plan.curtainEvery == 0){
            float bandTop = totalHeight;
            if(plan.usePodiumTower && !plan.useLShape && f < plan.podiumFloors){
                bandTop = pilotisHeight + plan.podiumFloors * plan.floorH;
            }
            if(AddCurtainWallBand(building, floorFp, z, bandTop, totalHeight, plan)){
                addedCurtain = true;
            }
        }

        AddBriseSoleil(building, floorFp, z, f, plan);
    }

    if(!addedCurtain && AllowsCurtain(plan)){
        float z0 = pilotisHeight + plan.slabT;
        float z1 = totalHeight - plan.roofCapT;
        if(z1 > z0 + 0.01f){
            Polygon2D cwFp = OutsetFromCentroid(fp.base, -plan.curtainInset);
            Mesh cw = BuildCurtainWall(cwFp,
                                       z0,
                                       z1,
                                       0.0f,
                                       plan.window,
                                       plan.concrete,
                                       2.2f,
                                       0.35f,
                                       0.15f);
            Append(building, cw);
        }
    }

    AddPodiumRoof(building, fp.base, pilotisHeight, plan);
    AddPilotis(building, fp.baseRect, fp.base, plan, pilotisHeight);

    Polygon2D capBase = fp.base;
    if(plan.usePodiumTower && !plan.useLShape && plan.podiumFloors < plan.totalFloors){
        capBase = fp.towerBase;
    }
    AddRoofCap(building, capBase, totalHeight, plan);

    Polygon2D deckBase = fp.base;
    if(plan.usePodiumTower && !plan.useLShape && plan.podiumFloors < plan.totalFloors){
        deckBase = fp.towerBase;
    }
    AddRoofDeck(building, deckBase, totalHeight, plan);

    return building;
}
