#include "semantics.h"

#include "config.h"

namespace sbl {

std::vector<SlabSemantic> BuildDefaultSlabPlan(int totalFloors,
                                               bool usePodiumTower,
                                               int podiumFloors)
{
    std::vector<SlabSemantic> plan;
    int nextFloor = 0;
    if(usePodiumTower && podiumFloors > 0){
        plan.push_back({SlabRole::Podium, nextFloor, podiumFloors});
        nextFloor += podiumFloors;
    }
    if(nextFloor < totalFloors){
        plan.push_back({SlabRole::Office, nextFloor, totalFloors - nextFloor});
    }
    plan.push_back({SlabRole::Roof, totalFloors, 1});
    return plan;
}

static BuildingSemantics DefaultBuildingSemantics(bool enablePilotis, bool usePodiumTower)
{
    BuildingSemantics sem{};
    sem.use = BuildingUse::Office;
    sem.urbanRole = UrbanRole::Infill;
    sem.placement = SitePlacement::CenteredObject;
    sem.ground = enablePilotis ? GroundInterface::Permeable : GroundInterface::Active;
    sem.massing = usePodiumTower ? MassingType::PodiumWithTower : MassingType::Tower;
    sem.hierarchy = usePodiumTower ? VerticalHierarchy::PodiumDominant : VerticalHierarchy::Uniform;
    sem.tone = VisualTone::Neutral;
    sem.contrast = ContrastLevel::Medium;
    if(enablePilotis){
        sem.env.push_back(EnvironmentalStrategy::FloodResilient);
    }
    return sem;
}

BuildingPlan BuildPlanFromConfig(const ::Config& cfg)
{
    BuildingPlan plan{};
    plan.style = cfg.style;
    plan.lot = cfg.lot;
    plan.lotShrink = cfg.lotShrink;
    plan.lotSnap = cfg.lotSnap;
    plan.lotBiasDir = {0.0f, 0.0f};
    plan.lotBias = 0.0f;
    plan.forceFootprintAspect = false;
    plan.footprintAspect = 1.0f;
    plan.totalFloors = cfg.floors;
    plan.floorH = cfg.floorH;
    plan.slabT = cfg.slabT;
    plan.glassInset = cfg.glassInset;
    plan.enablePilotis = cfg.enablePilotis;
    plan.pilotisHeight = cfg.pilotisHeight;
    plan.usePodiumTower = cfg.usePodiumTower;
    plan.podiumFloors = cfg.podiumFloors;
    plan.towerInset = cfg.towerInset;
    plan.useLShape = cfg.useLShape;
    plan.lCutX = cfg.lCutX;
    plan.lCutY = cfg.lCutY;
    plan.finEvery = cfg.finEvery;
    plan.finThickness = cfg.finThickness;
    plan.finProjection = cfg.finProjection;
    plan.roofCapT = cfg.roofCapT;
    plan.roofCapOverhang = cfg.roofCapOverhang;
    plan.roofDeckT = cfg.roofDeckT;
    plan.roofDeckInset = cfg.roofDeckInset;
    plan.roofDeckEnclosed = false;
    plan.curtainInset = cfg.curtainInset;
    plan.curtainEvery = cfg.curtainEvery;
    plan.curtainBandFloors = cfg.curtainBandFloors;
    plan.concrete = cfg.concrete;
    plan.window = cfg.window;
    plan.roofDeck = cfg.roofDeck;
    plan.lotFill = cfg.lotFill;
    plan.showLot = cfg.showLot;

    if(cfg.style == "brutalist"){
        plan.facadeType = FacadeType::Solid;
        plan.fenestration = FenestrationPattern::Punched;
    }else if(cfg.style == "bungalow"){
        plan.facadeType = FacadeType::Solid;
        plan.fenestration = FenestrationPattern::Punched;
    }else{
        plan.facadeType = FacadeType::BriseSoleil;
        plan.fenestration = FenestrationPattern::ContinuousBand;
    }
    plan.semantics = DefaultBuildingSemantics(cfg.enablePilotis, cfg.usePodiumTower);
    plan.slabPlan = BuildDefaultSlabPlan(plan.totalFloors, plan.usePodiumTower, plan.podiumFloors);

    return plan;
}

} // namespace sbl
