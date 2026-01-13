#include "style_midcentury.h"

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

void AddFloorSlab(Mesh& out, const Polygon2D& fp, float z, const Config& cfg, bool isTop)
{
    Vec3 slabColor = isTop ? cfg.roofDeck : cfg.concrete;
    Mesh slab = BuildSlab({fp,z,cfg.slabT},slabColor,0.02f);
    Append(out, slab);
}

void AddCurtainWallBand(Mesh& out,
                        const Polygon2D& fp,
                        float z,
                        float bandTop,
                        float totalHeight,
                        const Config& cfg)
{
    float cwTop = std::min(z + 3 * cfg.floorH, bandTop);
    cwTop = std::min(cwTop, totalHeight - cfg.roofCapT);
    if(cwTop <= z + cfg.slabT){
        return;
    }
    Polygon2D cwFp = OutsetFromCentroid(fp, -cfg.curtainInset);
    Mesh cw = BuildCurtainWall(cwFp,
                               z+cfg.slabT,
                               cwTop,
                               0.0f,
                               cfg.window,
                               cfg.concrete,
                               2.2f,
                               0.35f,
                               0.15f);
    Append(out, cw);
}

void AddBriseSoleil(Mesh& out, const Polygon2D& fp, float z, int f, const Config& cfg)
{
    if(cfg.finEvery <= 0 || (f + 1) % cfg.finEvery != 0 || f == cfg.floors - 1){
        return;
    }
    float finZ = z + cfg.floorH - cfg.finThickness * 0.5f;
    Polygon2D finFp = OutsetFromCentroid(fp, cfg.finProjection);
    Mesh fin = BuildSlab({finFp,finZ,cfg.finThickness},cfg.concrete,0.02f);
    Append(out, fin);
}

void AddPodiumRoof(Mesh& out, const Polygon2D& base, float pilotisHeight, const Config& cfg)
{
    if(!cfg.usePodiumTower || cfg.useLShape || cfg.podiumFloors <= 0 || cfg.podiumFloors >= cfg.floors){
        return;
    }
    float podiumZ = pilotisHeight + cfg.podiumFloors * cfg.floorH - 0.02f;
    Mesh podiumRoof = BuildSlab({base,podiumZ,0.25f},cfg.concrete,0.02f);
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
    float colHalf = 0.25f;

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
    Polygon2D capFp = cfg.useLShape ? capBase : OutsetFromCentroid(capBase, cfg.roofCapOverhang);
    float capZ = totalHeight - cfg.roofCapT;
    Mesh cap = BuildSlab({capFp,capZ,cfg.roofCapT},cfg.concrete,0.02f);
    Append(out, cap);
}

void AddRoofDeck(Mesh& out, const Polygon2D& deckBase, float totalHeight, const Config& cfg)
{
    Polygon2D deckFp = cfg.useLShape ? deckBase : OutsetFromCentroid(deckBase, -cfg.roofDeckInset);
    float deckZ = totalHeight + 0.02f;
    Mesh deck = BuildSlab({deckFp,deckZ,cfg.roofDeckT},cfg.roofDeck,0.02f);
    Append(out, deck);
}

} // namespace

Mesh BuildMidcenturyBuilding(const Config& cfg)
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
        AddFloorSlab(building, floorFp, z, cfg, f == cfg.floors - 1);

        if(f % 3 == 0){
            float bandTop = totalHeight;
            if(cfg.usePodiumTower && !cfg.useLShape && f < cfg.podiumFloors){
                bandTop = pilotisHeight + cfg.podiumFloors * cfg.floorH;
            }
            AddCurtainWallBand(building, floorFp, z, bandTop, totalHeight, cfg);
        }

        AddBriseSoleil(building, floorFp, z, f, cfg);
    }

    AddPodiumRoof(building, fp.base, pilotisHeight, cfg);
    AddPilotis(building, fp.baseRect, fp.base, cfg, pilotisHeight);

    Polygon2D capBase = fp.base;
    if(cfg.usePodiumTower && !cfg.useLShape && cfg.podiumFloors < cfg.floors){
        capBase = fp.towerBase;
    }
    AddRoofCap(building, capBase, totalHeight, cfg);

    Polygon2D deckBase = fp.base;
    if(cfg.usePodiumTower && !cfg.useLShape && cfg.podiumFloors < cfg.floors){
        deckBase = fp.towerBase;
    }
    AddRoofDeck(building, deckBase, totalHeight, cfg);

    return building;
}
