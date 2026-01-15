#include "city_builder.h"

#include <iostream>
#include <limits>
#include <random>

#include "config.h"
#include "geom_ops.h"
#include "geometry.h"
#include "semantics.h"
#include "style_brutalist.h"
#include "style_bungalow.h"
#include "style_midcentury.h"

namespace {

struct BuildStats {
    int buildingCount = 0;
    int parkCount = 0;
    int parkingCount = 0;
    int lotCount = 0;
    int treeCount = 0;
    int propCount = 0;
};

struct LotInstance {
    Polygon2D lot;
    Vec2 biasDir;
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
float DegToRad(float deg)
{
    return deg * 0.01745329252f;
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
    bool useFootprintSize = false;
    float footprintWidth = 0.0f;
    float footprintDepth = 0.0f;
    float bungalowWallHeight = 0.0f;
    float bungalowRoofHeight = 0.0f;
    std::string style;
};

struct BuildingFootprints {
    Polygon2D baseRect;
    Polygon2D base;
    Polygon2D towerBase;
};

bool LotCanFitFootprint(const Polygon2D& lot, const CityConfig& city, float minW, float minD)
{
    OBB2D obb = ComputeOBB(lot);
    float usableW = std::max(0.0f, obb.halfX * 2.0f - city.lotSetback * 2.0f);
    float usableD = std::max(0.0f, obb.halfY * 2.0f - city.lotSetback * 2.0f);
    bool fitsDirect = usableW >= minW && usableD >= minD;
    bool fitsSwap = usableW >= minD && usableD >= minW;
    return fitsDirect || fitsSwap;
}

static sbl::BuildingSemantics BuildBuildingSemantics(const CityConfig& city, const Polygon2D& lot, Rng& rng,
                                                     SemanticDecisions* decisions)
{
    SemanticDecisions local;
    if(!decisions){
        decisions = &local;
    }

    std::vector<std::string> stylePool = city.styles;
    if(stylePool.empty()){
        stylePool = {"midcentury", "brutalist", "bungalow"};
    }
    bool canFitBungalow = LotCanFitFootprint(lot, city, 24.0f, 28.0f);
    bool canFitOffice = LotCanFitFootprint(lot, city, 40.0f, 20.0f);
    std::vector<std::string> candidates;
    for(const auto& style : stylePool){
        if(style == "bungalow"){
            if(canFitBungalow){
                candidates.push_back(style);
            }
        }else if(canFitOffice){
            candidates.push_back(style);
        }
    }
    if(!candidates.empty()){
        int idx = rng.RangeInt(0, (int)candidates.size() - 1);
        decisions->style = candidates[idx];
    }else if(canFitBungalow){
        decisions->style = "bungalow";
    }else if(!stylePool.empty()){
        decisions->style = stylePool.front();
    }else{
        decisions->style = "midcentury";
    }

    bool isBungalow = (decisions->style == "bungalow");
    if(isBungalow){
        decisions->floors = 1;
        decisions->useFootprintSize = true;
        decisions->footprintWidth = rng.Range(20.0f, 32.0f);
        decisions->footprintDepth = rng.Range(24.0f, 36.0f);
        decisions->bungalowWallHeight = rng.Range(8.0f, 9.0f);
        float totalHeight = rng.Range(14.0f, 20.0f);
        decisions->bungalowRoofHeight = std::max(2.0f, totalHeight - decisions->bungalowWallHeight);
    }else{
        int lowMax = std::max(city.minFloors, std::min(city.lowMaxFloors, city.maxFloors));
        decisions->floors = rng.RangeInt(city.minFloors, city.maxFloors);
        if(!rng.Chance(city.tallChance)){
            decisions->floors = rng.RangeInt(city.minFloors, lowMax);
        }
        if(rng.Chance(0.35f)){
            int tallMin = std::min(city.maxFloors, std::max(lowMax + 1, city.minFloors));
            if(tallMin <= city.maxFloors){
                decisions->floors = rng.RangeInt(tallMin, city.maxFloors);
            }
        }
        decisions->useFootprintSize = true;
        decisions->footprintWidth = rng.Range(40.0f, 80.0f);
        decisions->footprintDepth = rng.Range(20.0f, 35.0f);
    }

    decisions->enablePilotis = isBungalow ? false : (city.disablePilotis ? false : rng.Chance(city.pilotisChance));

    decisions->useLShape = isBungalow ? false : rng.Chance(city.lShapeChance);
    if(decisions->useLShape){
        decisions->lCutX = rng.Range(4.0f, 8.0f);
        decisions->lCutY = rng.Range(3.0f, 7.0f);
    }

    decisions->usePodiumTower = isBungalow ? false : rng.Chance(0.45f);
    decisions->podiumFloors = decisions->usePodiumTower ? rng.RangeInt(2, 4) : 0;
    decisions->towerInset = decisions->usePodiumTower ? rng.Range(1.5f, 3.5f) : 0.0f;
    decisions->roofDeckEnclosed = isBungalow ? false : rng.Chance(0.35f);

    sbl::BuildingSemantics sem{};
    sem.use = isBungalow ? sbl::BuildingUse::Residential : sbl::BuildingUse::Office;
    sem.urbanRole = sbl::UrbanRole::Infill;
    sem.placement = sbl::SitePlacement::CenteredObject;
    sem.ground = decisions->enablePilotis ? sbl::GroundInterface::Permeable : sbl::GroundInterface::Active;
    sem.massing = isBungalow ? sbl::MassingType::Slab
                             : (decisions->usePodiumTower ? sbl::MassingType::PodiumWithTower : sbl::MassingType::Tower);
    sem.hierarchy = decisions->usePodiumTower ? sbl::VerticalHierarchy::PodiumDominant : sbl::VerticalHierarchy::Uniform;
    sem.tone = isBungalow ? sbl::VisualTone::Cheerful : sbl::VisualTone::Neutral;
    sem.contrast = isBungalow ? sbl::ContrastLevel::Low : sbl::ContrastLevel::Medium;
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
    plan.useFootprintSize = decisions.useFootprintSize;
    plan.footprintWidth = decisions.footprintWidth;
    plan.footprintDepth = decisions.footprintDepth;
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
    if(plan.style == "brutalist"){
        plan.facadeType = sbl::FacadeType::Solid;
        plan.fenestration = sbl::FenestrationPattern::Punched;
    }else if(plan.style == "bungalow"){
        plan.facadeType = sbl::FacadeType::Solid;
        plan.fenestration = sbl::FenestrationPattern::Punched;
    }else{
        plan.facadeType = sbl::FacadeType::BriseSoleil;
        plan.fenestration = sbl::FenestrationPattern::ContinuousBand;
    }
    plan.concrete = defaults.concrete;
    plan.window = defaults.window;
    plan.roofDeck = defaults.roofDeck;
    plan.lotFill = defaults.lotFill;
    plan.showLot = false;
    plan.roofHeightOverride = 0.0f;

    if(plan.style == "bungalow"){
        if(decisions.bungalowWallHeight > 0.0f){
            plan.floorH = decisions.bungalowWallHeight;
        }
        if(decisions.bungalowRoofHeight > 0.0f){
            plan.roofHeightOverride = decisions.bungalowRoofHeight;
        }
    }

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

struct LotRange {
    float widthMin;
    float widthMax;
    float depthMin;
    float depthMax;
};

LotRange PickLotRange(const CityConfig& cfg, Rng& rng)
{
    bool hasBungalow = false;
    for(const auto& style : cfg.styles){
        if(style == "bungalow"){
            hasBungalow = true;
            break;
        }
    }
    bool useRes = hasBungalow && cfg.resLotChance > 0.0f && rng.Chance(cfg.resLotChance);
    LotRange range{};
    if(useRes){
        range.widthMin = std::min(cfg.resLotWidth, cfg.resLotWidthMax);
        range.widthMax = std::max(cfg.resLotWidth, cfg.resLotWidthMax);
        range.depthMin = std::min(cfg.resLotDepth, cfg.resLotDepthMax);
        range.depthMax = std::max(cfg.resLotDepth, cfg.resLotDepthMax);
    }else{
        range.widthMin = std::min(cfg.lotWidth, cfg.lotWidthMax);
        range.widthMax = std::max(cfg.lotWidth, cfg.lotWidthMax);
        range.depthMin = std::min(cfg.lotDepth, cfg.lotDepthMax);
        range.depthMax = std::max(cfg.lotDepth, cfg.lotDepthMax);
    }
    return range;
}

void GetGlobalLotMins(const CityConfig& cfg, float* outWidthMin, float* outDepthMin)
{
    float widthMin = std::min(cfg.lotWidth, cfg.lotWidthMax);
    float depthMin = std::min(cfg.lotDepth, cfg.lotDepthMax);
    if(cfg.resLotWidth > 0.0f || cfg.resLotWidthMax > 0.0f){
        widthMin = std::min(widthMin, std::min(cfg.resLotWidth, cfg.resLotWidthMax));
    }
    if(cfg.resLotDepth > 0.0f || cfg.resLotDepthMax > 0.0f){
        depthMin = std::min(depthMin, std::min(cfg.resLotDepth, cfg.resLotDepthMax));
    }
    if(outWidthMin){
        *outWidthMin = widthMin;
    }
    if(outDepthMin){
        *outDepthMin = depthMin;
    }
}

void EmitLotsAlongXEdge(std::vector<LotInstance>* lots, const CityConfig& cfg, Rng& rng,
                        float startX, float endX, float edgeY, Vec2 inward, Vec2 biasDir)
{
    if(!lots){
        return;
    }
    float rotMax = DegToRad(std::max(0.0f, cfg.lotRotationMaxDeg));
    float x = startX;
    while(true){
        LotRange range = PickLotRange(cfg, rng);
        float widthMin = range.widthMin;
        float widthMax = range.widthMax;
        float depthMin = range.depthMin;
        float depthMax = range.depthMax;
        float depthLimit = (cfg.blockSizeY - cfg.sidewalk * 2.0f) * 0.5f;
        depthMax = std::min(depthMax, depthLimit);
        depthMin = std::min(depthMin, depthMax);
        if(widthMin <= 0.0f || depthMax <= 0.0f){
            return;
        }
        if(x + widthMin > endX + 1e-4f){
            break;
        }
        float remaining = endX - x;
        float widthMaxFit = std::min(widthMax, remaining);
        if(widthMaxFit < widthMin){
            break;
        }
        float width = rng.Range(widthMin, widthMaxFit);
        float depth = rng.Range(depthMin, depthMax);
        Vec2 center{x + width * 0.5f, edgeY + inward.y * depth * 0.5f};
        float rot = rotMax > 0.0f ? rng.Range(-rotMax, rotMax) : 0.0f;
        Polygon2D lot = MakeRectangle(center, width, depth, rot);
        lots->push_back({lot, biasDir});
        x += width + cfg.lotGap;
    }
}

void EmitLotsAlongYEdge(std::vector<LotInstance>* lots, const CityConfig& cfg, Rng& rng,
                        float startY, float endY, float edgeX, Vec2 inward, Vec2 biasDir)
{
    if(!lots){
        return;
    }
    float rotMax = DegToRad(std::max(0.0f, cfg.lotRotationMaxDeg));
    float baseAngle = 1.57079632679f;
    float y = startY;
    while(true){
        LotRange range = PickLotRange(cfg, rng);
        float widthMin = range.widthMin;
        float widthMax = range.widthMax;
        float depthMin = range.depthMin;
        float depthMax = range.depthMax;
        float depthLimit = (cfg.blockSizeX - cfg.sidewalk * 2.0f) * 0.5f;
        depthMax = std::min(depthMax, depthLimit);
        depthMin = std::min(depthMin, depthMax);
        if(widthMin <= 0.0f || depthMax <= 0.0f){
            return;
        }
        if(y + widthMin > endY + 1e-4f){
            break;
        }
        float remaining = endY - y;
        float widthMaxFit = std::min(widthMax, remaining);
        if(widthMaxFit < widthMin){
            break;
        }
        float width = rng.Range(widthMin, widthMaxFit);
        float depth = rng.Range(depthMin, depthMax);
        Vec2 center{edgeX + inward.x * depth * 0.5f, y + width * 0.5f};
        float rot = rotMax > 0.0f ? rng.Range(-rotMax, rotMax) : 0.0f;
        Polygon2D lot = MakeRectangle(center, width, depth, baseAngle + rot);
        lots->push_back({lot, biasDir});
        y += width + cfg.lotGap;
    }
}

std::vector<LotInstance> GenerateLotsForBlock(const CityConfig& cfg, float bx, float by, Rng& rng)
{
    std::vector<LotInstance> lots;
    float inset = cfg.sidewalk;
    float minWidth = 0.0f;
    float minDepth = 0.0f;
    GetGlobalLotMins(cfg, &minWidth, &minDepth);
    float depthLimitY = (cfg.blockSizeY - cfg.sidewalk * 2.0f) * 0.5f;
    float depthLimitX = (cfg.blockSizeX - cfg.sidewalk * 2.0f) * 0.5f;
    float cornerClearX = std::min(minDepth, depthLimitX) + cfg.lotGap;
    float cornerClearY = std::min(minDepth, depthLimitY) + cfg.lotGap;
    float usableX = std::max(0.0f, cfg.blockSizeX - inset * 2.0f);
    float usableY = std::max(0.0f, cfg.blockSizeY - inset * 2.0f);
    float maxCornerClearX = std::max(0.0f, (usableX - minWidth) * 0.5f);
    float maxCornerClearY = std::max(0.0f, (usableY - minWidth) * 0.5f);
    cornerClearX = std::min(cornerClearX, maxCornerClearX);
    cornerClearY = std::min(cornerClearY, maxCornerClearY);

    float startX = bx + inset + cornerClearX;
    float endX = bx + cfg.blockSizeX - inset - cornerClearX;
    float startY = by + inset + cornerClearY;
    float endY = by + cfg.blockSizeY - inset - cornerClearY;

    if(endX > startX){
        EmitLotsAlongXEdge(&lots, cfg, rng, startX, endX, by + inset, {0.0f, 1.0f}, {0.0f, -1.0f});
        EmitLotsAlongXEdge(&lots, cfg, rng, startX, endX, by + cfg.blockSizeY - inset, {0.0f, -1.0f}, {0.0f, 1.0f});
    }
    if(endY > startY){
        EmitLotsAlongYEdge(&lots, cfg, rng, startY, endY, bx + inset, {1.0f, 0.0f}, {-1.0f, 0.0f});
        EmitLotsAlongYEdge(&lots, cfg, rng, startY, endY, bx + cfg.blockSizeX - inset, {-1.0f, 0.0f}, {1.0f, 0.0f});
    }
    return lots;
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

void AddLots(Mesh& out, const CityConfig& cfg, const std::vector<LotInstance>& lots, BuildStats* stats)
{
    if(!cfg.showLots){
        return;
    }
    for(const auto& lot : lots){
        AddLotSlab(out, lot.lot, cfg.lotColor);
        if(stats){
            stats->lotCount++;
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

Mesh BuildBuildingForStyle(const sbl::BuildingPlan& plan)
{
    if(plan.style == "brutalist"){
        return BuildBrutalistBuilding(plan);
    }
    if(plan.style == "bungalow"){
        return BuildBungalowBuilding(plan);
    }
    return BuildMidcenturyBuilding(plan);
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

void AddBuildingsOnLots(Mesh& out, const CityConfig& cfg, const std::vector<LotInstance>& lots, Rng& rng,
                        BuildStats* stats, std::vector<TreeInstance>* trees,
                        std::vector<PropInstance>* props)
{
    const float parkZ = -0.12f;
    const float parkThickness = 0.08f;
    const float parkTop = parkZ + parkThickness;
    for(const auto& lot : lots){
        const Polygon2D& poly = lot.lot;
        if(rng.Chance(cfg.parkChance)){
            Mesh park = BuildSlab({poly, parkZ, parkThickness}, cfg.parkColor, 0.03f, SlabRole::Public);
            Append(out, park);
            if(stats) stats->parkCount++;
            EmitTreesInPark(poly, parkTop, cfg, rng, trees, stats);
            EmitGridPropsInPolygon(poly, parkTop, "bench",
                                   cfg.parkBenchSpacing, cfg.parkBenchInset,
                                   cfg.parkBenchJitter, cfg.parkBenchChance,
                                   rng, props, stats);
            continue;
        }
        if(rng.Chance(cfg.parkingChance)){
            Mesh park = BuildSlab({poly, parkZ, parkThickness}, cfg.parkingColor, 0.03f, SlabRole::Infrastructure);
            Append(out, park);
            if(stats) stats->parkingCount++;
            EmitGridPropsInPolygon(poly, parkTop, "car",
                                   cfg.parkingCarSpacing, cfg.parkingCarInset,
                                   cfg.parkingCarJitter, cfg.parkingCarChance,
                                   rng, props, stats);
            continue;
        }
        SemanticDecisions decisions;
        sbl::BuildingSemantics sem = BuildBuildingSemantics(cfg, poly, rng, &decisions);
        ApplyFootprintAspectDecision(poly, rng, &decisions);
        sbl::BuildingPlan plan = CompileBuildingPlan(cfg, poly, lot.biasDir, sem, decisions);
        std::cout << "Building " << plan.style
                  << " floors=" << plan.totalFloors
                  << " facade=" << FacadeLabel(plan.facadeType)
                  << "\n";
        Mesh ms = BuildBuildingForStyle(plan);
        Append(out, ms);
        if(stats) stats->buildingCount++;
        EmitRooftopProps(plan, rng, cfg, props, stats);
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

            std::vector<LotInstance> lots = GenerateLotsForBlock(cfg, blockX, blockY, rng);
            AddLots(result.mesh, cfg, lots, &stats);
            AddBuildingsOnLots(result.mesh, cfg, lots, rng, &stats, &result.trees, &result.props);
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
