#pragma once

#include <string>
#include <vector>

#include "math.h"

struct CityConfig {
    int seed;
    int blocksX;
    int blocksY;
    float blockSizeX;
    float blockSizeY;
    float roadWidth;
    float roadThickness;

    float lotDepth;
    float lotWidth;
    float lotGap;
    float sidewalk;
    float lotSetback;
    float parkChance;
    float parkingChance;

    int minFloors;
    int maxFloors;
    int lowMaxFloors;
    float tallChance;
    float pilotisChance;
    bool disablePilotis;
    float lShapeChance;

    std::vector<std::string> styles;
    std::vector<std::string> treeVariants;

    float treeSpacing;
    float treeInset;
    float treeJitter;
    float treeChance;
    bool emitTreesInGltf;

    float rooftopAcSpacing;
    float rooftopAcInset;
    float rooftopAcJitter;
    float rooftopAcChance;

    float streetBinSpacing;
    float streetBinOffset;
    float streetBinJitter;
    float streetBinChance;

    float streetLightSpacing;
    float streetLightOffset;
    float streetLightChance;

    float parkBenchSpacing;
    float parkBenchInset;
    float parkBenchJitter;
    float parkBenchChance;

    float parkingCarSpacing;
    float parkingCarInset;
    float parkingCarJitter;
    float parkingCarChance;

    bool emitPropsInGltf;

    Vec3 roadColor;
    Vec3 lotColor;
    Vec3 parkColor;
    Vec3 parkingColor;
    bool showLots;
};

CityConfig DefaultCityConfig();
bool LoadCityConfig(const char* path, CityConfig* out, std::string* err);
