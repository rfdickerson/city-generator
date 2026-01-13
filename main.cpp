#include <iostream>

#include "config.h"
#include "geometry.h"
#include "mesh.h"
#include "io.h"

int main(int argc, char** argv)
{
    Config cfg;
    std::string err;
    const char* configPath = (argc > 1) ? argv[1] : nullptr;
    if(!LoadConfig(configPath, &cfg, &err)){
        std::cerr << "Failed to load config: " << err << "\n";
        return 1;
    }

    // --- Building footprint from lot (OBB + shrink + snap + clamp) ---
    Polygon2D baseRect = PlaceRectInLot(cfg.lot, cfg.lotShrink, cfg.lotSnap);
    Polygon2D base = baseRect;
    if(cfg.useLShape){
        base = MakeLShapeFootprint(baseRect, cfg.lCutX, cfg.lCutY);
    }

    float pilotisHeight = cfg.enablePilotis ? cfg.pilotisHeight : 0.0f;

    float totalHeight = pilotisHeight + cfg.floors * cfg.floorH;

    Polygon2D towerBase = base;
    if(cfg.usePodiumTower && !cfg.useLShape && cfg.podiumFloors < cfg.floors){
        towerBase = base.Inset(cfg.towerInset);
    }

    Mesh building;
    unsigned offset = 0;

    // --- Lot visualization slab ---
    {
        Mesh lotMesh = BuildSlab({cfg.lot,-0.25f,0.25f},cfg.lotFill,0.03f);
        for(auto& i:lotMesh.i) i+=offset;
        building.v.insert(building.v.end(),lotMesh.v.begin(),lotMesh.v.end());
        building.i.insert(building.i.end(),lotMesh.i.begin(),lotMesh.i.end());
        offset += (unsigned)lotMesh.v.size();
    }

    for(int f=0;f<cfg.floors;f++){
        Polygon2D fp = base;
        if(cfg.usePodiumTower && !cfg.useLShape && f >= cfg.podiumFloors){
            fp = towerBase;
        }

        float z = pilotisHeight + f * cfg.floorH;

        // --- slab ---
        Vec3 slabColor = (f == cfg.floors-1) ? cfg.roofDeck : cfg.concrete;
        Mesh slab = BuildSlab({fp,z,cfg.slabT},slabColor,0.02f);
        for(auto& i:slab.i) i+=offset;
        building.v.insert(building.v.end(),slab.v.begin(),slab.v.end());
        building.i.insert(building.i.end(),slab.i.begin(),slab.i.end());
        offset += (unsigned)slab.v.size();

        // --- curtain wall every 3 floors ---
        if(f % 3 == 0){
            float bandTop = totalHeight;
            if(cfg.usePodiumTower && !cfg.useLShape && f < cfg.podiumFloors){
                bandTop = pilotisHeight + cfg.podiumFloors * cfg.floorH;
            }
            float cwTop = std::min(z + 3 * cfg.floorH, bandTop);
            cwTop = std::min(cwTop, totalHeight - cfg.roofCapT);
            if(cwTop > z + cfg.slabT){
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
                for(auto& i:cw.i) i+=offset;
                building.v.insert(building.v.end(),cw.v.begin(),cw.v.end());
                building.i.insert(building.i.end(),cw.i.begin(),cw.i.end());
                offset += (unsigned)cw.v.size();
            }
        }

        // --- Brise-soleil: horizontal fins every 2 floors ---
        if(cfg.finEvery > 0 && (f + 1) % cfg.finEvery == 0 && f != cfg.floors - 1){
            float finZ = z + cfg.floorH - cfg.finThickness * 0.5f;
            Polygon2D finFp = OutsetFromCentroid(fp, cfg.finProjection);
            Mesh fin = BuildSlab({finFp,finZ,cfg.finThickness},cfg.concrete,0.02f);
            for(auto& i:fin.i) i+=offset;
            building.v.insert(building.v.end(),fin.v.begin(),fin.v.end());
            building.i.insert(building.i.end(),fin.i.begin(),fin.i.end());
            offset += (unsigned)fin.v.size();
        }
    }

    // --- Podium roof slab to support tower footprint ---
    if(cfg.usePodiumTower && !cfg.useLShape && cfg.podiumFloors > 0 && cfg.podiumFloors < cfg.floors){
        float podiumZ = pilotisHeight + cfg.podiumFloors * cfg.floorH - 0.02f;
        Mesh podiumRoof = BuildSlab({base,podiumZ,0.25f},cfg.concrete,0.02f);
        for(auto& i:podiumRoof.i) i+=offset;
        building.v.insert(building.v.end(),podiumRoof.v.begin(),podiumRoof.v.end());
        building.i.insert(building.i.end(),podiumRoof.i.begin(),podiumRoof.i.end());
        offset += (unsigned)podiumRoof.v.size();
    }

    // --- Pilotis columns ---
    if(pilotisHeight > 0.0f){
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
                AddBox(building, c, axisX, axisY, colHalf, colHalf, 0.0f, pilotisHeight, cfg.concrete);
            }
        }
    }

    // --- Roof cap ---
    {
        Polygon2D capBase = base;
        if(cfg.usePodiumTower && !cfg.useLShape && cfg.podiumFloors < cfg.floors){
            capBase = towerBase;
        }
        Polygon2D capFp = cfg.useLShape ? capBase : OutsetFromCentroid(capBase, cfg.roofCapOverhang);
        float capZ = totalHeight - cfg.roofCapT;
        Mesh cap = BuildSlab({capFp,capZ,cfg.roofCapT},cfg.concrete,0.02f);
        for(auto& i:cap.i) i+=offset;
        building.v.insert(building.v.end(),cap.v.begin(),cap.v.end());
        building.i.insert(building.i.end(),cap.i.begin(),cap.i.end());
        offset += (unsigned)cap.v.size();
    }

    // --- Roof deck ---
    {
        Polygon2D deckBase = base;
        if(cfg.usePodiumTower && !cfg.useLShape && cfg.podiumFloors < cfg.floors){
            deckBase = towerBase;
        }
        Polygon2D deckFp = cfg.useLShape ? deckBase : OutsetFromCentroid(deckBase, -cfg.roofDeckInset);
        float deckZ = totalHeight + 0.02f;
        Mesh deck = BuildSlab({deckFp,deckZ,cfg.roofDeckT},cfg.roofDeck,0.02f);
        for(auto& i:deck.i) i+=offset;
        building.v.insert(building.v.end(),deck.v.begin(),deck.v.end());
        building.i.insert(building.i.end(),deck.i.begin(),deck.i.end());
        offset += (unsigned)deck.v.size();
    }

    WriteOBJ("simcity_midcentury_office.obj",building);
    WriteGLTF("simcity_midcentury_office.gltf",building);
    std::cout << "Wrote simcity_midcentury_office.obj and simcity_midcentury_office.gltf\n";
    return 0;
}
