#include "city_builder.h"

#include <iostream>
#include <limits>
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
    int treeCount = 0;
    int propCount = 0;
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

struct Bounds2 {
    float minX;
    float minY;
    float maxX;
    float maxY;
};

Bounds2 BoundsOf(const Polygon2D& poly)
{
    Bounds2 b{std::numeric_limits<float>::max(),
              std::numeric_limits<float>::max(),
              -std::numeric_limits<float>::max(),
              -std::numeric_limits<float>::max()};
    for(const auto& p : poly.v){
        b.minX = std::min(b.minX, p.x);
        b.minY = std::min(b.minY, p.y);
        b.maxX = std::max(b.maxX, p.x);
        b.maxY = std::max(b.maxY, p.y);
    }
    return b;
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
    bool forceFootprintAspect = false;
    float footprintAspect = 1.0f;
    bool roofDeckEnclosed = false;
    std::string style;
};

struct BuildingFootprints {
    Polygon2D baseRect;
    Polygon2D base;
    Polygon2D towerBase;
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
    if(decisions->useLShape){
        decisions->lCutX = rng.Range(4.0f, 8.0f);
        decisions->lCutY = rng.Range(3.0f, 7.0f);
    }

    decisions->usePodiumTower = false;
    decisions->podiumFloors = DefaultConfig().podiumFloors;
    decisions->towerInset = DefaultConfig().towerInset;
    decisions->roofDeckEnclosed = rng.Chance(0.35f);

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

static void ApplyFootprintAspectDecision(const Polygon2D& lot, Rng& rng, SemanticDecisions* decisions)
{
    if(!decisions){
        return;
    }

    OBB2D obb = ComputeOBB(lot);
    if(obb.halfX <= 1e-4f || obb.halfY <= 1e-4f){
        return;
    }

    float aspect = std::max(obb.halfX, obb.halfY) / std::min(obb.halfX, obb.halfY);
    float chance = (aspect > 1.2f) ? 0.6f : 0.35f;
    if(rng.Chance(chance)){
        decisions->forceFootprintAspect = true;
        decisions->footprintAspect = 2.0f;
    }
}

static sbl::BuildingPlan CompileBuildingPlan(const CityConfig& city, const Polygon2D& lot,
                                             Vec2 lotBiasDir,
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
    plan.lotBiasDir = lotBiasDir;
    plan.lotBias = (Length(lotBiasDir) > 1e-4f) ? 0.7f : 0.0f;
    plan.forceFootprintAspect = decisions.forceFootprintAspect;
    plan.footprintAspect = decisions.footprintAspect;
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
    plan.roofDeckEnclosed = decisions.roofDeckEnclosed;
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

BuildingFootprints ComputeBuildingFootprints(const sbl::BuildingPlan& plan)
{
    BuildingFootprints fp;
    fp.baseRect = PlaceRectInLot(plan.lot, plan.lotShrink, plan.lotSnap, plan.lotBiasDir, plan.lotBias);
    if(plan.forceFootprintAspect && plan.footprintAspect > 1.01f){
        float minShortHalf = std::max(2.0f, plan.lotSnap * 2.0f);
        fp.baseRect = EnforceRectAspect(fp.baseRect, plan.footprintAspect, minShortHalf);
    }
    fp.base = fp.baseRect;
    if(plan.useLShape){
        fp.base = MakeLShapeFootprint(fp.baseRect, plan.lCutX, plan.lCutY);
    }
    fp.towerBase = fp.base;
    if(plan.usePodiumTower && !plan.useLShape && plan.podiumFloors < plan.totalFloors){
        fp.towerBase = fp.base.Inset(plan.towerInset);
    }
    return fp;
}

float ComputePilotisHeight(const sbl::BuildingPlan& plan)
{
    return plan.enablePilotis ? plan.pilotisHeight : 0.0f;
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

void EmitTreesInPark(const Polygon2D& park, float groundY, const CityConfig& cfg,
                     Rng& rng, std::vector<TreeInstance>* trees, BuildStats* stats)
{
    if(!trees || cfg.treeVariants.empty()){
        return;
    }
    if(cfg.treeChance <= 0.0f){
        return;
    }

    float spacing = std::max(0.1f, cfg.treeSpacing);
    float inset = std::max(0.0f, cfg.treeInset);
    float jitter = std::max(0.0f, cfg.treeJitter);

    Bounds2 b = BoundsOf(park);
    float minX = b.minX + inset;
    float maxX = b.maxX - inset;
    float minY = b.minY + inset;
    float maxY = b.maxY - inset;
    if(maxX <= minX || maxY <= minY){
        return;
    }

    for(float x = minX; x <= maxX + 1e-4f; x += spacing){
        for(float y = minY; y <= maxY + 1e-4f; y += spacing){
            if(cfg.treeChance < 1.0f && !rng.Chance(cfg.treeChance)){
                continue;
            }
            Vec2 p{x, y};
            if(jitter > 0.0f){
                p.x += rng.Range(-jitter, jitter);
                p.y += rng.Range(-jitter, jitter);
                p.x = std::max(minX, std::min(maxX, p.x));
                p.y = std::max(minY, std::min(maxY, p.y));
            }
            if(!PointInPolygon(park, p)){
                continue;
            }
            int variantIndex = rng.RangeInt(0, (int)cfg.treeVariants.size() - 1);
            TreeInstance inst;
            inst.position = {p.x, groundY, p.y};
            inst.variant = cfg.treeVariants[variantIndex];
            trees->push_back(inst);
            if(stats) stats->treeCount++;
        }
    }
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

void EmitGridPropsInPolygon(const Polygon2D& area, float height, const char* type,
                            float spacing, float inset, float jitter, float chance,
                            Rng& rng, std::vector<PropInstance>* props, BuildStats* stats)
{
    if(!props || !type){
        return;
    }
    if(chance <= 0.0f || spacing <= 0.0f){
        return;
    }
    float safeSpacing = std::max(0.1f, spacing);
    float safeInset = std::max(0.0f, inset);
    float safeJitter = std::max(0.0f, jitter);

    Bounds2 b = BoundsOf(area);
    float minX = b.minX + safeInset;
    float maxX = b.maxX - safeInset;
    float minY = b.minY + safeInset;
    float maxY = b.maxY - safeInset;
    if(maxX <= minX || maxY <= minY){
        return;
    }

    for(float x = minX; x <= maxX + 1e-4f; x += safeSpacing){
        for(float y = minY; y <= maxY + 1e-4f; y += safeSpacing){
            if(chance < 1.0f && !rng.Chance(chance)){
                continue;
            }
            Vec2 p{x, y};
            if(safeJitter > 0.0f){
                p.x += rng.Range(-safeJitter, safeJitter);
                p.y += rng.Range(-safeJitter, safeJitter);
                p.x = std::max(minX, std::min(maxX, p.x));
                p.y = std::max(minY, std::min(maxY, p.y));
            }
            if(!PointInPolygon(area, p)){
                continue;
            }
            PropInstance inst;
            inst.position = {p.x, height, p.y};
            inst.type = type;
            props->push_back(inst);
            if(stats) stats->propCount++;
        }
    }
}

void EmitStreetLineProps(Vec2 start, Vec2 end, Vec2 inward, float offset,
                         float spacing, float jitter, float chance, const char* type,
                         Rng& rng, std::vector<PropInstance>* props, BuildStats* stats)
{
    if(!props || !type){
        return;
    }
    if(chance <= 0.0f || spacing <= 0.0f){
        return;
    }
    Vec2 dir = Normalize({end.x - start.x, end.y - start.y});
    float length = Length({end.x - start.x, end.y - start.y});
    if(length <= 0.01f){
        return;
    }
    float safeSpacing = std::max(0.1f, spacing);
    float safeJitter = std::max(0.0f, jitter);
    float safeOffset = std::max(0.0f, offset);
    float t = safeSpacing * 0.5f;
    while(t <= length - safeSpacing * 0.5f + 1e-4f){
        if(chance < 1.0f && !rng.Chance(chance)){
            t += safeSpacing;
            continue;
        }
        float jitterOffset = safeJitter > 0.0f ? rng.Range(-safeJitter, safeJitter) : 0.0f;
        float sampleT = std::max(0.0f, std::min(length, t + jitterOffset));
        Vec2 p = start + dir * sampleT + inward * safeOffset;
        PropInstance inst;
        inst.position = {p.x, 0.0f, p.y};
        inst.type = type;
        props->push_back(inst);
        if(stats) stats->propCount++;
        t += safeSpacing;
    }
}

void EmitStreetPropsForBlock(float bx, float by, const CityConfig& cfg, Rng& rng,
                             std::vector<PropInstance>* props, BuildStats* stats)
{
    float minX = bx;
    float maxX = bx + cfg.blockSizeX;
    float minY = by;
    float maxY = by + cfg.blockSizeY;

    Vec2 bl{minX, minY};
    Vec2 br{maxX, minY};
    Vec2 tl{minX, maxY};
    Vec2 tr{maxX, maxY};

    EmitStreetLineProps(bl, br, {0.0f, 1.0f}, cfg.streetBinOffset,
                        cfg.streetBinSpacing, cfg.streetBinJitter, cfg.streetBinChance,
                        "trash_bin", rng, props, stats);
    EmitStreetLineProps(tl, tr, {0.0f, -1.0f}, cfg.streetBinOffset,
                        cfg.streetBinSpacing, cfg.streetBinJitter, cfg.streetBinChance,
                        "trash_bin", rng, props, stats);
    EmitStreetLineProps(bl, tl, {1.0f, 0.0f}, cfg.streetBinOffset,
                        cfg.streetBinSpacing, cfg.streetBinJitter, cfg.streetBinChance,
                        "trash_bin", rng, props, stats);
    EmitStreetLineProps(br, tr, {-1.0f, 0.0f}, cfg.streetBinOffset,
                        cfg.streetBinSpacing, cfg.streetBinJitter, cfg.streetBinChance,
                        "trash_bin", rng, props, stats);

    EmitStreetLineProps(bl, br, {0.0f, 1.0f}, cfg.streetLightOffset,
                        cfg.streetLightSpacing, 0.0f, cfg.streetLightChance,
                        "street_light", rng, props, stats);
    EmitStreetLineProps(tl, tr, {0.0f, -1.0f}, cfg.streetLightOffset,
                        cfg.streetLightSpacing, 0.0f, cfg.streetLightChance,
                        "street_light", rng, props, stats);
    EmitStreetLineProps(bl, tl, {1.0f, 0.0f}, cfg.streetLightOffset,
                        cfg.streetLightSpacing, 0.0f, cfg.streetLightChance,
                        "street_light", rng, props, stats);
    EmitStreetLineProps(br, tr, {-1.0f, 0.0f}, cfg.streetLightOffset,
                        cfg.streetLightSpacing, 0.0f, cfg.streetLightChance,
                        "street_light", rng, props, stats);
}

void EmitRooftopProps(const sbl::BuildingPlan& plan, Rng& rng, const CityConfig& cfg,
                      std::vector<PropInstance>* props, BuildStats* stats)
{
    if(!props){
        return;
    }
    BuildingFootprints fp = ComputeBuildingFootprints(plan);
    Polygon2D roofBase = fp.base;
    if(plan.usePodiumTower && !plan.useLShape && plan.podiumFloors < plan.totalFloors){
        roofBase = fp.towerBase;
    }
    float pilotisHeight = ComputePilotisHeight(plan);
    float totalHeight = pilotisHeight + plan.totalFloors * plan.floorH;

    EmitGridPropsInPolygon(roofBase, totalHeight, "ac_unit",
                           cfg.rooftopAcSpacing, cfg.rooftopAcInset,
                           cfg.rooftopAcJitter, cfg.rooftopAcChance,
                           rng, props, stats);
}

void AddBuildingsOnLots(Mesh& out, const CityConfig& cfg, float bx, float by, Rng& rng,
                        BuildStats* stats, std::vector<TreeInstance>* trees,
                        std::vector<PropInstance>* props)
{
    float inset = cfg.sidewalk;
    float usableW = cfg.blockSizeX - inset * 2.0f;
    float usableH = cfg.blockSizeY - inset * 2.0f;
    const float parkZ = -0.12f;
    const float parkThickness = 0.08f;
    const float parkTop = parkZ + parkThickness;

    int countX = std::max(1, (int)((usableW + cfg.lotGap) / (cfg.lotWidth + cfg.lotGap)));
    int countY = std::max(1, (int)((usableH + cfg.lotGap) / (cfg.lotWidth + cfg.lotGap)));

    for(int i=0;i<countX;i++){
        float x = bx + inset + i * (cfg.lotWidth + cfg.lotGap);
        Polygon2D south = MakeRect(x, by + inset, cfg.lotWidth, cfg.lotDepth);
        Polygon2D north = MakeRect(x, by + cfg.blockSizeY - inset - cfg.lotDepth, cfg.lotWidth, cfg.lotDepth);

        if(rng.Chance(cfg.parkChance)){
            Mesh park = BuildSlab({south, parkZ, parkThickness}, cfg.parkColor, 0.03f, SlabRole::Public);
            Append(out, park);
            if(stats) stats->parkCount++;
            EmitTreesInPark(south, parkTop, cfg, rng, trees, stats);
            EmitGridPropsInPolygon(south, parkTop, "bench",
                                   cfg.parkBenchSpacing, cfg.parkBenchInset,
                                   cfg.parkBenchJitter, cfg.parkBenchChance,
                                   rng, props, stats);
        }else if(rng.Chance(cfg.parkingChance)){
            Mesh park = BuildSlab({south, parkZ, parkThickness}, cfg.parkingColor, 0.03f, SlabRole::Infrastructure);
            Append(out, park);
            if(stats) stats->parkingCount++;
            EmitGridPropsInPolygon(south, parkTop, "car",
                                   cfg.parkingCarSpacing, cfg.parkingCarInset,
                                   cfg.parkingCarJitter, cfg.parkingCarChance,
                                   rng, props, stats);
        }else{
            SemanticDecisions decisions;
            sbl::BuildingSemantics sem = BuildBuildingSemantics(cfg, rng, &decisions);
            ApplyFootprintAspectDecision(south, rng, &decisions);
            sbl::BuildingPlan plan = CompileBuildingPlan(cfg, south, {0.0f, -1.0f}, sem, decisions);
            std::cout << "Building " << plan.style
                      << " floors=" << plan.totalFloors
                      << " facade=" << FacadeLabel(plan.facadeType)
                      << "\n";
            Mesh ms = (plan.style == "brutalist") ? BuildBrutalistBuilding(plan) : BuildMidcenturyBuilding(plan);
            Append(out, ms);
            if(stats) stats->buildingCount++;
            EmitRooftopProps(plan, rng, cfg, props, stats);
        }

        if(rng.Chance(cfg.parkChance)){
            Mesh park = BuildSlab({north, parkZ, parkThickness}, cfg.parkColor, 0.03f, SlabRole::Public);
            Append(out, park);
            if(stats) stats->parkCount++;
            EmitTreesInPark(north, parkTop, cfg, rng, trees, stats);
            EmitGridPropsInPolygon(north, parkTop, "bench",
                                   cfg.parkBenchSpacing, cfg.parkBenchInset,
                                   cfg.parkBenchJitter, cfg.parkBenchChance,
                                   rng, props, stats);
        }else if(rng.Chance(cfg.parkingChance)){
            Mesh park = BuildSlab({north, parkZ, parkThickness}, cfg.parkingColor, 0.03f, SlabRole::Infrastructure);
            Append(out, park);
            if(stats) stats->parkingCount++;
            EmitGridPropsInPolygon(north, parkTop, "car",
                                   cfg.parkingCarSpacing, cfg.parkingCarInset,
                                   cfg.parkingCarJitter, cfg.parkingCarChance,
                                   rng, props, stats);
        }else{
            SemanticDecisions decisions;
            sbl::BuildingSemantics sem = BuildBuildingSemantics(cfg, rng, &decisions);
            ApplyFootprintAspectDecision(north, rng, &decisions);
            sbl::BuildingPlan plan = CompileBuildingPlan(cfg, north, {0.0f, 1.0f}, sem, decisions);
            std::cout << "Building " << plan.style
                      << " floors=" << plan.totalFloors
                      << " facade=" << FacadeLabel(plan.facadeType)
                      << "\n";
            Mesh mn = (plan.style == "brutalist") ? BuildBrutalistBuilding(plan) : BuildMidcenturyBuilding(plan);
            Append(out, mn);
            if(stats) stats->buildingCount++;
            EmitRooftopProps(plan, rng, cfg, props, stats);
        }
    }

    for(int j=0;j<countY;j++){
        float y = by + inset + j * (cfg.lotWidth + cfg.lotGap);
        Polygon2D west = MakeRect(bx + inset, y, cfg.lotDepth, cfg.lotWidth);
        Polygon2D east = MakeRect(bx + cfg.blockSizeX - inset - cfg.lotDepth, y, cfg.lotDepth, cfg.lotWidth);

        if(rng.Chance(cfg.parkChance)){
            Mesh park = BuildSlab({west, parkZ, parkThickness}, cfg.parkColor, 0.03f, SlabRole::Public);
            Append(out, park);
            if(stats) stats->parkCount++;
            EmitTreesInPark(west, parkTop, cfg, rng, trees, stats);
            EmitGridPropsInPolygon(west, parkTop, "bench",
                                   cfg.parkBenchSpacing, cfg.parkBenchInset,
                                   cfg.parkBenchJitter, cfg.parkBenchChance,
                                   rng, props, stats);
        }else if(rng.Chance(cfg.parkingChance)){
            Mesh park = BuildSlab({west, parkZ, parkThickness}, cfg.parkingColor, 0.03f, SlabRole::Infrastructure);
            Append(out, park);
            if(stats) stats->parkingCount++;
            EmitGridPropsInPolygon(west, parkTop, "car",
                                   cfg.parkingCarSpacing, cfg.parkingCarInset,
                                   cfg.parkingCarJitter, cfg.parkingCarChance,
                                   rng, props, stats);
        }else{
            SemanticDecisions decisions;
            sbl::BuildingSemantics sem = BuildBuildingSemantics(cfg, rng, &decisions);
            ApplyFootprintAspectDecision(west, rng, &decisions);
            sbl::BuildingPlan plan = CompileBuildingPlan(cfg, west, {-1.0f, 0.0f}, sem, decisions);
            std::cout << "Building " << plan.style
                      << " floors=" << plan.totalFloors
                      << " facade=" << FacadeLabel(plan.facadeType)
                      << "\n";
            Mesh mw = (plan.style == "brutalist") ? BuildBrutalistBuilding(plan) : BuildMidcenturyBuilding(plan);
            Append(out, mw);
            if(stats) stats->buildingCount++;
            EmitRooftopProps(plan, rng, cfg, props, stats);
        }

        if(rng.Chance(cfg.parkChance)){
            Mesh park = BuildSlab({east, parkZ, parkThickness}, cfg.parkColor, 0.03f, SlabRole::Public);
            Append(out, park);
            if(stats) stats->parkCount++;
            EmitTreesInPark(east, parkTop, cfg, rng, trees, stats);
            EmitGridPropsInPolygon(east, parkTop, "bench",
                                   cfg.parkBenchSpacing, cfg.parkBenchInset,
                                   cfg.parkBenchJitter, cfg.parkBenchChance,
                                   rng, props, stats);
        }else if(rng.Chance(cfg.parkingChance)){
            Mesh park = BuildSlab({east, parkZ, parkThickness}, cfg.parkingColor, 0.03f, SlabRole::Infrastructure);
            Append(out, park);
            if(stats) stats->parkingCount++;
            EmitGridPropsInPolygon(east, parkTop, "car",
                                   cfg.parkingCarSpacing, cfg.parkingCarInset,
                                   cfg.parkingCarJitter, cfg.parkingCarChance,
                                   rng, props, stats);
        }else{
            SemanticDecisions decisions;
            sbl::BuildingSemantics sem = BuildBuildingSemantics(cfg, rng, &decisions);
            ApplyFootprintAspectDecision(east, rng, &decisions);
            sbl::BuildingPlan plan = CompileBuildingPlan(cfg, east, {1.0f, 0.0f}, sem, decisions);
            std::cout << "Building " << plan.style
                      << " floors=" << plan.totalFloors
                      << " facade=" << FacadeLabel(plan.facadeType)
                      << "\n";
            Mesh me = (plan.style == "brutalist") ? BuildBrutalistBuilding(plan) : BuildMidcenturyBuilding(plan);
            Append(out, me);
            if(stats) stats->buildingCount++;
            EmitRooftopProps(plan, rng, cfg, props, stats);
        }
    }
}

} // namespace

CityBuild BuildCity(const CityConfig& cfg)
{
    float totalW = cfg.blocksX * cfg.blockSizeX + (cfg.blocksX + 1) * cfg.roadWidth;
    float totalH = cfg.blocksY * cfg.blockSizeY + (cfg.blocksY + 1) * cfg.roadWidth;

    CityBuild result;
    std::cout << "City build start blocks=" << cfg.blocksX << "x" << cfg.blocksY
              << " seed=" << cfg.seed << "\n";
    AddRoads(result.mesh, cfg, totalW, totalH);
    std::cout << "Roads generated\n";

    Rng rng(cfg.seed);
    BuildStats stats;

    for(int by=0;by<cfg.blocksY;by++){
        for(int bx=0;bx<cfg.blocksX;bx++){
            float blockX = cfg.roadWidth + bx * (cfg.blockSizeX + cfg.roadWidth);
            float blockY = cfg.roadWidth + by * (cfg.blockSizeY + cfg.roadWidth);

            AddLots(result.mesh, cfg, blockX, blockY, &stats);
            AddBuildingsOnLots(result.mesh, cfg, blockX, blockY, rng, &stats, &result.trees, &result.props);
            EmitStreetPropsForBlock(blockX, blockY, cfg, rng, &result.props, &stats);
        }
    }

    std::cout << "City build done buildings=" << stats.buildingCount
              << " parks=" << stats.parkCount
              << " parking=" << stats.parkingCount
              << " lotSlabs=" << stats.lotCount
              << " trees=" << stats.treeCount
              << " props=" << stats.propCount
              << "\n";
    return result;
}

Mesh BuildCityMesh(const CityConfig& cfg)
{
    CityBuild result = BuildCity(cfg);
    return result.mesh;
}
