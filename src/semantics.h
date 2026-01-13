#pragma once

#include <string>
#include <vector>

#include "geometry.h"

struct Config;

namespace sbl {

enum class BuildingUse {
    Office,
    Residential,
    Hotel,
    Civic,
    MixedUse,
    Industrial,
    Parking
};

enum class UrbanRole {
    Landmark,
    EdgeDefiner,
    Infill,
    Gateway,
    Anchor,
    Utility
};

enum class SitePlacement {
    CenteredObject,
    EdgeAligned,
    CornerEmphasis,
    Pavilion,
    PodiumAndTower
};

enum class GroundInterface {
    Active,
    Permeable,
    Elevated,
    Sealed
};

enum class MassingType {
    Slab,
    Tower,
    Courtyard,
    Stepped,
    Terraced,
    StackedVolumes,
    PodiumWithTower
};

enum class VerticalHierarchy {
    Uniform,
    PodiumDominant,
    TowerDominant,
    BaseMiddleCrown
};

enum class SlabRole {
    Infrastructure,
    Podium,
    Public,
    Office,
    Residential,
    Terrace,
    Mechanical,
    Roof
};

enum class FacadeType {
    Solid,
    CurtainWall,
    Recessed,
    Screened,
    BriseSoleil
};

enum class FenestrationPattern {
    ContinuousBand,
    VerticalRhythm,
    HorizontalRhythm,
    Punched,
    None
};

enum class EnvironmentalStrategy {
    None,
    Shaded,
    GreenTerrace,
    SolarRoof,
    PassiveCooling,
    FloodResilient
};

enum class VisualTone {
    Cheerful,
    Neutral,
    Formal,
    Monumental,
    Playful
};

enum class ContrastLevel {
    Low,
    Medium,
    High
};

enum class MaterialIntent {
    Concrete,
    Glass,
    Stone,
    Metal,
    Greenery
};

struct BuildingSemantics {
    BuildingUse use;
    UrbanRole urbanRole;
    SitePlacement placement;
    GroundInterface ground;
    MassingType massing;
    VerticalHierarchy hierarchy;
    VisualTone tone;
    ContrastLevel contrast;
    std::vector<EnvironmentalStrategy> env;
};

struct SlabSemantic {
    SlabRole role;
    int startFloor;
    int floorCount;
};

struct BuildingPlan {
    BuildingSemantics semantics;
    std::vector<SlabSemantic> slabPlan;
    Polygon2D lot;
    std::string style;
    float lotShrink;
    float lotSnap;
    Vec2 lotBiasDir;
    float lotBias;
    int totalFloors;
    float floorH;
    float slabT;
    float glassInset;
    bool enablePilotis;
    float pilotisHeight;
    bool usePodiumTower;
    int podiumFloors;
    float towerInset;
    bool useLShape;
    float lCutX;
    float lCutY;
    int finEvery;
    float finThickness;
    float finProjection;
    float roofCapT;
    float roofCapOverhang;
    float roofDeckT;
    float roofDeckInset;
    float curtainInset;
    int curtainEvery;
    int curtainBandFloors;
    FacadeType facadeType;
    FenestrationPattern fenestration;
    Vec3 concrete;
    Vec3 window;
    Vec3 roofDeck;
    Vec3 lotFill;
    bool showLot;
};

std::vector<SlabSemantic> BuildDefaultSlabPlan(int totalFloors,
                                               bool usePodiumTower,
                                               int podiumFloors);

BuildingPlan BuildPlanFromConfig(const ::Config& cfg);

} // namespace sbl
