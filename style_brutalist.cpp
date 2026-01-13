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

Footprints ComputeFootprints(const Config& cfg)
{
    Footprints f;
    f.baseRect = PlaceRectInLot(cfg.lot, cfg.lotShrink, cfg.lotSnap);
    f.base = f.baseRect;
    if(cfg.useLShape){
        f.base = MakeLShapeFootprint(f.baseRect, cfg.lCutX, cfg.lCutY);
    }
    f.towerBase = f.base;
    if(cfg.usePodiumTower && !cfg.useLShape && cfg.podiumFloors < cfg.floors){
        f.towerBase = f.base.Inset(cfg.towerInset);
    }
    return f;
}

float ComputePilotisHeight(const Config& cfg)
{
    return cfg.enablePilotis ? cfg.pilotisHeight : 0.0f;
}

void AddLotMesh(Mesh& out, const Config& cfg)
{
    Mesh lotMesh = BuildSlab({cfg.lot,-0.25f,0.25f},cfg.lotFill,0.03f);
    Append(out, lotMesh);
}

void AddFloorSlab(Mesh& out, const Polygon2D& fp, float z, const Config& cfg)
{
    Mesh slab = BuildSlab({fp,z,cfg.slabT},cfg.concrete,0.02f);
    Append(out, slab);
}

void AddPodiumRoof(Mesh& out, const Polygon2D& base, float pilotisHeight, const Config& cfg)
{
    if(!cfg.usePodiumTower || cfg.useLShape || cfg.podiumFloors <= 0 || cfg.podiumFloors >= cfg.floors){
        return;
    }
    float podiumZ = pilotisHeight + cfg.podiumFloors * cfg.floorH - 0.02f;
    Mesh podiumRoof = BuildSlab({base,podiumZ,0.35f},cfg.concrete,0.02f);
    Append(out, podiumRoof);
}

void AddPilotis(Mesh& out, const Polygon2D& baseRect, const Polygon2D& base, const Config& cfg, float pilotisHeight)
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
            AddBox(out, c, axisX, axisY, colHalf, colHalf, 0.0f, pilotisHeight, cfg.concrete);
        }
    }
}

void AddRoofCap(Mesh& out, const Polygon2D& capBase, float totalHeight, const Config& cfg)
{
    float capT = std::max(cfg.roofCapT, cfg.slabT * 1.25f);
    Polygon2D capFp = cfg.useLShape ? capBase : OutsetFromCentroid(capBase, cfg.roofCapOverhang);
    float capZ = totalHeight - capT;
    Mesh cap = BuildSlab({capFp,capZ,capT},cfg.concrete,0.02f);
    Append(out, cap);
}

} // namespace

Mesh BuildBrutalistBuilding(const Config& cfg)
{
    Footprints fp = ComputeFootprints(cfg);
    float pilotisHeight = ComputePilotisHeight(cfg);
    float totalHeight = pilotisHeight + cfg.floors * cfg.floorH;

    Mesh building;
    AddLotMesh(building, cfg);

    for(int f=0;f<cfg.floors;f++){
        Polygon2D floorFp = fp.base;
        if(cfg.usePodiumTower && !cfg.useLShape && f >= cfg.podiumFloors){
            floorFp = fp.towerBase;
        }

        float z = pilotisHeight + f * cfg.floorH;
        AddFloorSlab(building, floorFp, z, cfg);
    }

    AddPodiumRoof(building, fp.base, pilotisHeight, cfg);
    AddPilotis(building, fp.baseRect, fp.base, cfg, pilotisHeight);

    Polygon2D capBase = fp.base;
    if(cfg.usePodiumTower && !cfg.useLShape && cfg.podiumFloors < cfg.floors){
        capBase = fp.towerBase;
    }
    AddRoofCap(building, capBase, totalHeight, cfg);

    return building;
}
