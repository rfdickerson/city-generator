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

    Vec3 roadColor;
    Vec3 lotColor;
    Vec3 parkColor;
    Vec3 parkingColor;
    bool showLots;
};

CityConfig DefaultCityConfig();
bool LoadCityConfig(const char* path, CityConfig* out, std::string* err);
