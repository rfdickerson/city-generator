#pragma once

#include <string>
#include "geometry.h"

struct Config {
    std::string style;
    std::string outputName;
    Polygon2D lot;
    float lotShrink;
    float lotSnap;
    bool showLot;

    int floors;
    float floorH;
    float slabT;
    float glassInset;

    bool enablePilotis;
    float pilotisHeight;

    int finEvery;
    float finThickness;
    float finProjection;

    bool usePodiumTower;
    int podiumFloors;
    float towerInset;

    bool useLShape;
    float lCutX;
    float lCutY;

    float roofCapT;
    float roofCapOverhang;
    float roofDeckT;
    float roofDeckInset;

    float curtainInset;
    int curtainEvery;
    int curtainBandFloors;

    Vec3 concrete;
    Vec3 window;
    Vec3 roofDeck;
    Vec3 lotFill;
};

Config DefaultConfig();
bool LoadConfig(const char* path, Config* out, std::string* err);
