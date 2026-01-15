#include <gtest/gtest.h>

#include <algorithm>

#include "building_compiler.h"
#include "building_model.h"
#include "geometry.h"

namespace {

Polygon2D MakeSquare(float minX, float minY, float maxX, float maxY)
{
    return {{
        {minX, minY},
        {maxX, minY},
        {maxX, maxY},
        {minX, maxY}
    }};
}

} // namespace

TEST(BuildingCompiler, RoofCompilationEmitsTriangles)
{
    BuildingModel model;
    Polygon2D base = MakeSquare(-2.0f, -2.0f, 2.0f, 2.0f);
    RoofParams roof{};
    roof.type = RoofType::Gable;
    roof.pitchDeg = 30.0f;
    roof.overhang = 0.5f;
    roof.ridgeHeight = 1.8f;
    roof.hipRidgeFrac = 0.5f;
    roof.aoStrength = 0.75f;
    AddRoofVolume(model, base, 3.0f, roof, {0.6f, 0.6f, 0.6f}, 0.1f);

    Mesh mesh = CompileBuildingModel(model);
    EXPECT_FALSE(mesh.v.empty());
    EXPECT_FALSE(mesh.i.empty());
    EXPECT_EQ(mesh.i.size() % 3, 0u);
    for(const auto& v : mesh.v){
        EXPECT_GE(v.pos.y, 3.0f);
    }
}

TEST(BuildingCompiler, SlabAndRoofCombine)
{
    BuildingModel model;
    Polygon2D base = MakeSquare(-1.5f, -1.0f, 1.5f, 1.0f);
    AddSlabVolume(model, base, 0.0f, 2.0f, {0.5f, 0.5f, 0.5f}, 0.05f, SlabRole::Residential);

    RoofParams roof{};
    roof.type = RoofType::Hip;
    roof.pitchDeg = 26.0f;
    roof.overhang = 0.4f;
    roof.ridgeHeight = 1.2f;
    roof.hipRidgeFrac = 0.35f;
    roof.aoStrength = 0.6f;
    AddRoofVolume(model, base, 2.0f, roof, {0.4f, 0.4f, 0.45f}, 0.05f);

    Mesh mesh = CompileBuildingModel(model);
    EXPECT_GT(mesh.v.size(), 12u);
    EXPECT_GT(mesh.i.size(), 12u);
    EXPECT_EQ(mesh.i.size() % 3, 0u);
    float minY = mesh.v.front().pos.y;
    float maxY = mesh.v.front().pos.y;
    for(const auto& v : mesh.v){
        minY = std::min(minY, v.pos.y);
        maxY = std::max(maxY, v.pos.y);
    }
    EXPECT_LE(minY, 0.0f);
    EXPECT_GT(maxY, 2.0f);
}
