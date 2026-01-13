#include "city_builder.h"

#include <iostream>
#include <random>

#include "config.h"
#include "geom_ops.h"
#include "geometry.h"
#include "semantics.h"
#include "style_midcentury.h"
#include "style_brutalist.h"

namespace {

struct BuildStats {
    int buildingCount = 0;
    int parkCount = 0;
    int parkingCount = 0;
    int lotCount = 0;
};

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

struct SemanticDecisions {
    int floors = 0;
    bool enablePilotis = false;
    bool usePodiumTower = false;
    int podiumFloors = 0;
    float towerInset = 0.0f;
    bool useLShape = false;
    float lCutX = 0.0f;
    float lCutY = 0.0f;
    std::string style;
};

static sbl::BuildingSemantics BuildBuildingSemantics(const CityConfig& city, Rng& rng,
                                                     SemanticDecisions* decisions)
{
    SemanticDecisions local;
    if(!decisions){
        decisions = &local;
    }

    decisions->style = "midcentury";
    if(!city.styles.empty()){
        int idx = rng.RangeInt(0, (int)city.styles.size() - 1);
        decisions->style = city.styles[idx];
    }

    int lowMax = std::max(city.minFloors, std::min(city.lowMaxFloors, city.maxFloors));
    decisions->floors = rng.RangeInt(city.minFloors, city.maxFloors);
    if(!rng.Chance(city.tallChance)){
        decisions->floors = rng.RangeInt(city.minFloors, lowMax);
    }

    decisions->enablePilotis = city.disablePilotis ? false : rng.Chance(city.pilotisChance);

    decisions->useLShape = rng.Chance(city.lShapeChance);
    if(decisions->style == "midcentury"){
        decisions->useLShape = false;
    }
    if(decisions->useLShape){
        decisions->lCutX = rng.Range(4.0f, 8.0f);
        decisions->lCutY = rng.Range(3.0f, 7.0f);
    }

    decisions->usePodiumTower = false;
    decisions->podiumFloors = DefaultConfig().podiumFloors;
    decisions->towerInset = DefaultConfig().towerInset;

    sbl::BuildingSemantics sem{};
    sem.use = sbl::BuildingUse::Office;
    sem.urbanRole = sbl::UrbanRole::Infill;
    sem.placement = sbl::SitePlacement::CenteredObject;
    sem.ground = decisions->enablePilotis ? sbl::GroundInterface::Permeable : sbl::GroundInterface::Active;
    sem.massing = decisions->usePodiumTower ? sbl::MassingType::PodiumWithTower : sbl::MassingType::Tower;
    sem.hierarchy = decisions->usePodiumTower ? sbl::VerticalHierarchy::PodiumDominant : sbl::VerticalHierarchy::Uniform;
    sem.tone = sbl::VisualTone::Neutral;
    sem.contrast = sbl::ContrastLevel::Medium;
    if(decisions->enablePilotis){
        sem.env.push_back(sbl::EnvironmentalStrategy::FloodResilient);
    }

    return sem;
}

static sbl::BuildingPlan CompileBuildingPlan(const CityConfig& city, const Polygon2D& lot,
                                             const sbl::BuildingSemantics& sem,
                                             const SemanticDecisions& decisions)
{
    Config defaults = DefaultConfig();

    sbl::BuildingPlan plan{};
    plan.semantics = sem;
    plan.lot = lot;
    plan.style = decisions.style;
    plan.lotShrink = city.lotSetback;
    plan.lotSnap = 0.5f;
    plan.totalFloors = decisions.floors;
    plan.floorH = defaults.floorH;
    plan.slabT = defaults.slabT;
    plan.glassInset = defaults.glassInset;
    plan.enablePilotis = (sem.ground == sbl::GroundInterface::Permeable);
    plan.pilotisHeight = defaults.pilotisHeight;
    plan.usePodiumTower = decisions.usePodiumTower;
    plan.podiumFloors = decisions.podiumFloors;
    plan.towerInset = decisions.towerInset;
    plan.useLShape = decisions.useLShape;
    plan.lCutX = decisions.lCutX;
    plan.lCutY = decisions.lCutY;
    plan.finEvery = defaults.finEvery;
    plan.finThickness = defaults.finThickness;
    plan.finProjection = defaults.finProjection;
    plan.roofCapT = defaults.roofCapT;
    plan.roofCapOverhang = defaults.roofCapOverhang;
    plan.roofDeckT = defaults.roofDeckT;
    plan.roofDeckInset = defaults.roofDeckInset;
    plan.curtainInset = defaults.curtainInset;
    plan.curtainEvery = 1;
    plan.curtainBandFloors = 1;
    plan.facadeType = (plan.style == "brutalist") ? sbl::FacadeType::Solid : sbl::FacadeType::BriseSoleil;
    plan.fenestration = (plan.style == "brutalist") ? sbl::FenestrationPattern::Punched
                                                    : sbl::FenestrationPattern::ContinuousBand;
    plan.concrete = defaults.concrete;
    plan.window = defaults.window;
    plan.roofDeck = defaults.roofDeck;
    plan.lotFill = defaults.lotFill;
    plan.showLot = false;

    plan.slabPlan = sbl::BuildDefaultSlabPlan(plan.totalFloors, plan.usePodiumTower, plan.podiumFloors);

    return plan;
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

void AddLots(Mesh& out, const CityConfig& cfg, float bx, float by, BuildStats* stats)
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
        if(stats){
            stats->lotCount += 2;
        }
    }

    for(int j=0;j<countY;j++){
        float y = by + inset + j * (cfg.lotWidth + cfg.lotGap);
        Polygon2D west = MakeRect(bx + inset, y, cfg.lotDepth, cfg.lotWidth);
        Polygon2D east = MakeRect(bx + cfg.blockSizeX - inset - cfg.lotDepth, y, cfg.lotDepth, cfg.lotWidth);
        AddLotSlab(out, west, cfg.lotColor);
        AddLotSlab(out, east, cfg.lotColor);
        if(stats){
            stats->lotCount += 2;
        }
    }
}

const char* FacadeLabel(sbl::FacadeType facade)
{
    switch(facade){
    case sbl::FacadeType::Solid:
        return "solid";
    case sbl::FacadeType::CurtainWall:
        return "curtain";
    case sbl::FacadeType::Recessed:
        return "recessed";
    case sbl::FacadeType::Screened:
        return "screened";
    case sbl::FacadeType::BriseSoleil:
        return "brise";
    default:
        return "unknown";
    }
}

void AddBuildingsOnLots(Mesh& out, const CityConfig& cfg, float bx, float by, Rng& rng, BuildStats* stats)
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
            if(stats) stats->parkCount++;
        }else if(rng.Chance(cfg.parkingChance)){
            Mesh park = BuildSlab({south,-0.12f,0.08f}, cfg.parkingColor, 0.03f, SlabRole::Infrastructure);
            Append(out, park);
            if(stats) stats->parkingCount++;
        }else{
            SemanticDecisions decisions;
            sbl::BuildingSemantics sem = BuildBuildingSemantics(cfg, rng, &decisions);
            sbl::BuildingPlan plan = CompileBuildingPlan(cfg, south, sem, decisions);
            std::cout << "Building " << plan.style
                      << " floors=" << plan.totalFloors
                      << " facade=" << FacadeLabel(plan.facadeType)
                      << "\n";
            Mesh ms = (plan.style == "brutalist") ? BuildBrutalistBuilding(plan) : BuildMidcenturyBuilding(plan);
            Append(out, ms);
            if(stats) stats->buildingCount++;
        }

        if(rng.Chance(cfg.parkChance)){
            Mesh park = BuildSlab({north,-0.12f,0.08f}, cfg.parkColor, 0.03f, SlabRole::Public);
            Append(out, park);
            if(stats) stats->parkCount++;
        }else if(rng.Chance(cfg.parkingChance)){
            Mesh park = BuildSlab({north,-0.12f,0.08f}, cfg.parkingColor, 0.03f, SlabRole::Infrastructure);
            Append(out, park);
            if(stats) stats->parkingCount++;
        }else{
            SemanticDecisions decisions;
            sbl::BuildingSemantics sem = BuildBuildingSemantics(cfg, rng, &decisions);
            sbl::BuildingPlan plan = CompileBuildingPlan(cfg, north, sem, decisions);
            std::cout << "Building " << plan.style
                      << " floors=" << plan.totalFloors
                      << " facade=" << FacadeLabel(plan.facadeType)
                      << "\n";
            Mesh mn = (plan.style == "brutalist") ? BuildBrutalistBuilding(plan) : BuildMidcenturyBuilding(plan);
            Append(out, mn);
            if(stats) stats->buildingCount++;
        }
    }

    for(int j=0;j<countY;j++){
        float y = by + inset + j * (cfg.lotWidth + cfg.lotGap);
        Polygon2D west = MakeRect(bx + inset, y, cfg.lotDepth, cfg.lotWidth);
        Polygon2D east = MakeRect(bx + cfg.blockSizeX - inset - cfg.lotDepth, y, cfg.lotDepth, cfg.lotWidth);

        if(rng.Chance(cfg.parkChance)){
            Mesh park = BuildSlab({west,-0.12f,0.08f}, cfg.parkColor, 0.03f, SlabRole::Public);
            Append(out, park);
            if(stats) stats->parkCount++;
        }else if(rng.Chance(cfg.parkingChance)){
            Mesh park = BuildSlab({west,-0.12f,0.08f}, cfg.parkingColor, 0.03f, SlabRole::Infrastructure);
            Append(out, park);
            if(stats) stats->parkingCount++;
        }else{
            SemanticDecisions decisions;
            sbl::BuildingSemantics sem = BuildBuildingSemantics(cfg, rng, &decisions);
            sbl::BuildingPlan plan = CompileBuildingPlan(cfg, west, sem, decisions);
            std::cout << "Building " << plan.style
                      << " floors=" << plan.totalFloors
                      << " facade=" << FacadeLabel(plan.facadeType)
                      << "\n";
            Mesh mw = (plan.style == "brutalist") ? BuildBrutalistBuilding(plan) : BuildMidcenturyBuilding(plan);
            Append(out, mw);
            if(stats) stats->buildingCount++;
        }

        if(rng.Chance(cfg.parkChance)){
            Mesh park = BuildSlab({east,-0.12f,0.08f}, cfg.parkColor, 0.03f, SlabRole::Public);
            Append(out, park);
            if(stats) stats->parkCount++;
        }else if(rng.Chance(cfg.parkingChance)){
            Mesh park = BuildSlab({east,-0.12f,0.08f}, cfg.parkingColor, 0.03f, SlabRole::Infrastructure);
            Append(out, park);
            if(stats) stats->parkingCount++;
        }else{
            SemanticDecisions decisions;
            sbl::BuildingSemantics sem = BuildBuildingSemantics(cfg, rng, &decisions);
            sbl::BuildingPlan plan = CompileBuildingPlan(cfg, east, sem, decisions);
            std::cout << "Building " << plan.style
                      << " floors=" << plan.totalFloors
                      << " facade=" << FacadeLabel(plan.facadeType)
                      << "\n";
            Mesh me = (plan.style == "brutalist") ? BuildBrutalistBuilding(plan) : BuildMidcenturyBuilding(plan);
            Append(out, me);
            if(stats) stats->buildingCount++;
        }
    }
}

} // namespace

Mesh BuildCityMesh(const CityConfig& cfg)
{
    float totalW = cfg.blocksX * cfg.blockSizeX + (cfg.blocksX + 1) * cfg.roadWidth;
    float totalH = cfg.blocksY * cfg.blockSizeY + (cfg.blocksY + 1) * cfg.roadWidth;

    Mesh city;
    std::cout << "City build start blocks=" << cfg.blocksX << "x" << cfg.blocksY
              << " seed=" << cfg.seed << "\n";
    AddRoads(city, cfg, totalW, totalH);
    std::cout << "Roads generated\n";

    Rng rng(cfg.seed);
    BuildStats stats;

    for(int by=0;by<cfg.blocksY;by++){
        for(int bx=0;bx<cfg.blocksX;bx++){
            float blockX = cfg.roadWidth + bx * (cfg.blockSizeX + cfg.roadWidth);
            float blockY = cfg.roadWidth + by * (cfg.blockSizeY + cfg.roadWidth);

            AddLots(city, cfg, blockX, blockY, &stats);
            AddBuildingsOnLots(city, cfg, blockX, blockY, rng, &stats);
        }
    }

    std::cout << "City build done buildings=" << stats.buildingCount
              << " parks=" << stats.parkCount
              << " parking=" << stats.parkingCount
              << " lotSlabs=" << stats.lotCount
              << "\n";
    return city;
}
