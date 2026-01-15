#pragma once

#include <vector>

#include "geometry.h"
#include "mesh.h"

enum class RoofType {
    Gable,
    Hip
};

struct RoofParams {
    RoofType type;
    float pitchDeg;
    float overhang;
    float ridgeHeight;
    float hipRidgeFrac;
    float aoStrength;
};

struct SlabVolume {
    Polygon2D footprint;
    float z;
    float thickness;
    Vec3 color;
    float uvScale;
    SlabRole role;
};

struct CurtainWallVolume {
    Polygon2D footprint;
    float z0;
    float z1;
    float inset;
    Vec3 windowColor;
    Vec3 mullionColor;
    float panelWidth;
    float mullionWidth;
    float uvVScale;
};

struct BoxVolume {
    Vec2 center;
    Vec2 axisX;
    Vec2 axisY;
    float halfX;
    float halfY;
    float z0;
    float z1;
    Vec3 color;
};

struct RoofVolume {
    Polygon2D footprint;
    float baseZ;
    RoofParams params;
    Vec3 color;
    float uvScale;
};

struct BuildingModel {
    std::vector<SlabVolume> slabs;
    std::vector<CurtainWallVolume> curtains;
    std::vector<BoxVolume> boxes;
    std::vector<RoofVolume> roofs;
};

inline void AddSlabVolume(BuildingModel& model,
                          const Polygon2D& footprint,
                          float z,
                          float thickness,
                          Vec3 color,
                          float uvScale,
                          SlabRole role)
{
    model.slabs.push_back({footprint, z, thickness, color, uvScale, role});
}

inline void AddCurtainWallVolume(BuildingModel& model,
                                 const Polygon2D& footprint,
                                 float z0,
                                 float z1,
                                 float inset,
                                 Vec3 windowColor,
                                 Vec3 mullionColor,
                                 float panelWidth,
                                 float mullionWidth,
                                 float uvVScale)
{
    model.curtains.push_back({footprint, z0, z1, inset, windowColor, mullionColor,
                              panelWidth, mullionWidth, uvVScale});
}

inline void AddBoxVolume(BuildingModel& model,
                         Vec2 center,
                         Vec2 axisX,
                         Vec2 axisY,
                         float halfX,
                         float halfY,
                         float z0,
                         float z1,
                         Vec3 color)
{
    model.boxes.push_back({center, axisX, axisY, halfX, halfY, z0, z1, color});
}

inline void AddRoofVolume(BuildingModel& model,
                          const Polygon2D& footprint,
                          float baseZ,
                          const RoofParams& params,
                          Vec3 color,
                          float uvScale)
{
    model.roofs.push_back({footprint, baseZ, params, color, uvScale});
}
