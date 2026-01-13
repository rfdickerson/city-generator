#include "city_builder.h"

#include <random>

#include "config.h"
#include "geom_ops.h"
#include "geometry.h"
#include "style_midcentury.h"
#include "style_brutalist.h"

namespace {

struct Rng {
    std::mt19937 gen;

    explicit Rng(int seed) : gen((std::mt19937::result_type)seed) {}

    int RangeInt(int a, int b){
        std::uniform_int_distribution<int> dist(a, b);
        return dist(gen);
    }

    float Range(float a, float b){
        std::uniform_real_distribution<float> dist(a, b);
        return dist(gen);
    }

    bool Chance(float p){
        std::bernoulli_distribution dist(p);
        return dist(gen);
    }
};

Polygon2D MakeRect(float x, float y, float w, float h)
{
    Vec2 center{x + w * 0.5f, y + h * 0.5f};
    return MakeRectangle(center, w, h, 0.0f);
}

void AddRoads(Mesh& out, const CityConfig& cfg, float totalW, float totalH)
{
    for(int y=0;y<=cfg.blocksY;y++){
        float ry = y * (cfg.blockSizeY + cfg.roadWidth);
        Polygon2D road = MakeRect(0.0f, ry, totalW, cfg.roadWidth);
        Mesh slab = BuildSlab({road, -0.1f, cfg.roadThickness}, cfg.roadColor, 0.02f, SlabRole::Infrastructure);
        Append(out, slab);
    }

    for(int x=0;x<=cfg.blocksX;x++){
        float rx = x * (cfg.blockSizeX + cfg.roadWidth);
        Polygon2D road = MakeRect(rx, 0.0f, cfg.roadWidth, totalH);
        Mesh slab = BuildSlab({road, -0.1f, cfg.roadThickness}, cfg.roadColor, 0.02f, SlabRole::Infrastructure);
        Append(out, slab);
    }
}

void AddLotSlab(Mesh& out, const Polygon2D& lot, Vec3 color)
{
    Mesh slab = BuildSlab({lot,-0.12f,0.08f}, color, 0.03f, SlabRole::Public);
    Append(out, slab);
}

void AddLots(Mesh& out, const CityConfig& cfg, float bx, float by)
{
    if(!cfg.showLots){
        return;
    }
    float inset = cfg.sidewalk;
    float usableW = cfg.blockSizeX - inset * 2.0f;
    float usableH = cfg.blockSizeY - inset * 2.0f;

    int countX = std::max(1, (int)((usableW + cfg.lotGap) / (cfg.lotWidth + cfg.lotGap)));
    int countY = std::max(1, (int)((usableH + cfg.lotGap) / (cfg.lotWidth + cfg.lotGap)));

    for(int i=0;i<countX;i++){
        float x = bx + inset + i * (cfg.lotWidth + cfg.lotGap);
        Polygon2D south = MakeRect(x, by + inset, cfg.lotWidth, cfg.lotDepth);
        Polygon2D north = MakeRect(x, by + cfg.blockSizeY - inset - cfg.lotDepth, cfg.lotWidth, cfg.lotDepth);
        AddLotSlab(out, south, cfg.lotColor);
        AddLotSlab(out, north, cfg.lotColor);
    }

    for(int j=0;j<countY;j++){
        float y = by + inset + j * (cfg.lotWidth + cfg.lotGap);
        Polygon2D west = MakeRect(bx + inset, y, cfg.lotDepth, cfg.lotWidth);
        Polygon2D east = MakeRect(bx + cfg.blockSizeX - inset - cfg.lotDepth, y, cfg.lotDepth, cfg.lotWidth);
        AddLotSlab(out, west, cfg.lotColor);
        AddLotSlab(out, east, cfg.lotColor);
    }
}

Config BuildBuildingConfig(const CityConfig& city, const Polygon2D& lot, Rng& rng)
{
    Config cfg = DefaultConfig();
    cfg.lot = lot;
    cfg.lotShrink = city.lotSetback;
    cfg.lotSnap = 0.5f;
    cfg.showLot = false;

    if(!city.styles.empty()){
        int idx = rng.RangeInt(0, (int)city.styles.size() - 1);
        cfg.style = city.styles[idx];
    }

    cfg.floors = rng.RangeInt(city.minFloors, city.maxFloors);
    int lowMax = std::max(city.minFloors, std::min(city.lowMaxFloors, city.maxFloors));
    if(!rng.Chance(city.tallChance)){
        cfg.floors = rng.RangeInt(city.minFloors, lowMax);
    }
    cfg.enablePilotis = city.disablePilotis ? false : rng.Chance(city.pilotisChance);
    cfg.useLShape = rng.Chance(city.lShapeChance);
    if(cfg.style == "midcentury"){
        cfg.useLShape = false;
    }
    cfg.curtainEvery = 1;
    cfg.curtainBandFloors = 1;

    if(cfg.useLShape){
        cfg.lCutX = rng.Range(4.0f, 8.0f);
        cfg.lCutY = rng.Range(3.0f, 7.0f);
    }

    return cfg;
}

void AddBuildingsOnLots(Mesh& out, const CityConfig& cfg, float bx, float by, Rng& rng)
{
    float inset = cfg.sidewalk;
    float usableW = cfg.blockSizeX - inset * 2.0f;
    float usableH = cfg.blockSizeY - inset * 2.0f;

    int countX = std::max(1, (int)((usableW + cfg.lotGap) / (cfg.lotWidth + cfg.lotGap)));
    int countY = std::max(1, (int)((usableH + cfg.lotGap) / (cfg.lotWidth + cfg.lotGap)));

    for(int i=0;i<countX;i++){
        float x = bx + inset + i * (cfg.lotWidth + cfg.lotGap);
        Polygon2D south = MakeRect(x, by + inset, cfg.lotWidth, cfg.lotDepth);
        Polygon2D north = MakeRect(x, by + cfg.blockSizeY - inset - cfg.lotDepth, cfg.lotWidth, cfg.lotDepth);

        if(rng.Chance(cfg.parkChance)){
            Mesh park = BuildSlab({south,-0.12f,0.08f}, cfg.parkColor, 0.03f, SlabRole::Public);
            Append(out, park);
        }else if(rng.Chance(cfg.parkingChance)){
            Mesh park = BuildSlab({south,-0.12f,0.08f}, cfg.parkingColor, 0.03f, SlabRole::Infrastructure);
            Append(out, park);
        }else{
            Config cs = BuildBuildingConfig(cfg, south, rng);
            Mesh ms = (cs.style == "brutalist") ? BuildBrutalistBuilding(cs) : BuildMidcenturyBuilding(cs);
            Append(out, ms);
        }

        if(rng.Chance(cfg.parkChance)){
            Mesh park = BuildSlab({north,-0.12f,0.08f}, cfg.parkColor, 0.03f, SlabRole::Public);
            Append(out, park);
        }else if(rng.Chance(cfg.parkingChance)){
            Mesh park = BuildSlab({north,-0.12f,0.08f}, cfg.parkingColor, 0.03f, SlabRole::Infrastructure);
            Append(out, park);
        }else{
            Config cn = BuildBuildingConfig(cfg, north, rng);
            Mesh mn = (cn.style == "brutalist") ? BuildBrutalistBuilding(cn) : BuildMidcenturyBuilding(cn);
            Append(out, mn);
        }
    }

    for(int j=0;j<countY;j++){
        float y = by + inset + j * (cfg.lotWidth + cfg.lotGap);
        Polygon2D west = MakeRect(bx + inset, y, cfg.lotDepth, cfg.lotWidth);
        Polygon2D east = MakeRect(bx + cfg.blockSizeX - inset - cfg.lotDepth, y, cfg.lotDepth, cfg.lotWidth);

        if(rng.Chance(cfg.parkChance)){
            Mesh park = BuildSlab({west,-0.12f,0.08f}, cfg.parkColor, 0.03f, SlabRole::Public);
            Append(out, park);
        }else if(rng.Chance(cfg.parkingChance)){
            Mesh park = BuildSlab({west,-0.12f,0.08f}, cfg.parkingColor, 0.03f, SlabRole::Infrastructure);
            Append(out, park);
        }else{
            Config cw = BuildBuildingConfig(cfg, west, rng);
            Mesh mw = (cw.style == "brutalist") ? BuildBrutalistBuilding(cw) : BuildMidcenturyBuilding(cw);
            Append(out, mw);
        }

        if(rng.Chance(cfg.parkChance)){
            Mesh park = BuildSlab({east,-0.12f,0.08f}, cfg.parkColor, 0.03f, SlabRole::Public);
            Append(out, park);
        }else if(rng.Chance(cfg.parkingChance)){
            Mesh park = BuildSlab({east,-0.12f,0.08f}, cfg.parkingColor, 0.03f, SlabRole::Infrastructure);
            Append(out, park);
        }else{
            Config ce = BuildBuildingConfig(cfg, east, rng);
            Mesh me = (ce.style == "brutalist") ? BuildBrutalistBuilding(ce) : BuildMidcenturyBuilding(ce);
            Append(out, me);
        }
    }
}

} // namespace

Mesh BuildCityMesh(const CityConfig& cfg)
{
    float totalW = cfg.blocksX * cfg.blockSizeX + (cfg.blocksX + 1) * cfg.roadWidth;
    float totalH = cfg.blocksY * cfg.blockSizeY + (cfg.blocksY + 1) * cfg.roadWidth;

    Mesh city;
    AddRoads(city, cfg, totalW, totalH);

    Rng rng(cfg.seed);

    for(int by=0;by<cfg.blocksY;by++){
        for(int bx=0;bx<cfg.blocksX;bx++){
            float blockX = cfg.roadWidth + bx * (cfg.blockSizeX + cfg.roadWidth);
            float blockY = cfg.roadWidth + by * (cfg.blockSizeY + cfg.roadWidth);

            AddLots(city, cfg, blockX, blockY);
            AddBuildingsOnLots(city, cfg, blockX, blockY, rng);
        }
    }

    return city;
}
