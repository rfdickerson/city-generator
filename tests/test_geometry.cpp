#include <gtest/gtest.h>

#include <algorithm>

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

float TriangleArea(Vec2 a, Vec2 b, Vec2 c)
{
    return 0.5f * std::fabs(Cross({b.x - a.x, b.y - a.y}, {c.x - a.x, c.y - a.y}));
}

float PolygonAreaAbs(const Polygon2D& poly)
{
    return std::fabs(SignedArea(poly.v));
}

} // namespace

TEST(Geometry, CentroidOfSquare)
{
    Polygon2D square = MakeSquare(0.0f, 0.0f, 2.0f, 2.0f);
    Vec2 c = Centroid(square);
    EXPECT_NEAR(c.x, 1.0f, 1e-5f);
    EXPECT_NEAR(c.y, 1.0f, 1e-5f);
}

TEST(Geometry, SignedAreaOrientation)
{
    Polygon2D square = MakeSquare(0.0f, 0.0f, 2.0f, 2.0f);
    EXPECT_NEAR(SignedArea(square.v), 4.0f, 1e-5f);
    std::reverse(square.v.begin(), square.v.end());
    EXPECT_NEAR(SignedArea(square.v), -4.0f, 1e-5f);
}

TEST(Geometry, PointInConvexCCWDetectsInside)
{
    Polygon2D tri{{{0.0f, 0.0f}, {2.0f, 0.0f}, {0.0f, 2.0f}}};
    EXPECT_TRUE(PointInConvexCCW(tri, {0.25f, 0.25f}));
    EXPECT_FALSE(PointInConvexCCW(tri, {1.5f, 1.5f}));
}

TEST(Geometry, PointInPolygonConcave)
{
    Polygon2D l{{{0.0f, 0.0f}, {3.0f, 0.0f}, {3.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 3.0f}, {0.0f, 3.0f}}};
    EXPECT_TRUE(PointInPolygon(l, {0.5f, 0.5f}));
    EXPECT_FALSE(PointInPolygon(l, {2.0f, 2.0f}));
}

TEST(Geometry, TriangulateCCWProducesValidTris)
{
    Polygon2D square = MakeSquare(0.0f, 0.0f, 2.0f, 2.0f);
    std::vector<unsigned> tris = TriangulateCCW(square.v);
    ASSERT_EQ(tris.size(), 6u);
    float area = 0.0f;
    for(size_t i = 0; i < tris.size(); i += 3){
        unsigned ia = tris[i];
        unsigned ib = tris[i + 1];
        unsigned ic = tris[i + 2];
        ASSERT_LT(ia, square.v.size());
        ASSERT_LT(ib, square.v.size());
        ASSERT_LT(ic, square.v.size());
        area += TriangleArea(square.v[ia], square.v[ib], square.v[ic]);
    }
    EXPECT_NEAR(area, 4.0f, 1e-4f);
}

TEST(Geometry, ComputeOBBMatchesAxisAlignedRect)
{
    Polygon2D rect = MakeSquare(0.0f, 0.0f, 4.0f, 2.0f);
    OBB2D obb = ComputeOBB(rect);
    EXPECT_NEAR(obb.halfX * 2.0f, 4.0f, 1e-3f);
    EXPECT_NEAR(obb.halfY * 2.0f, 2.0f, 1e-3f);
    EXPECT_NEAR(std::fabs(obb.axisX.x), 1.0f, 1e-3f);
    EXPECT_NEAR(std::fabs(obb.axisX.y), 0.0f, 1e-3f);
    EXPECT_NEAR(std::fabs(obb.axisY.x), 0.0f, 1e-3f);
    EXPECT_NEAR(std::fabs(obb.axisY.y), 1.0f, 1e-3f);
}

TEST(Geometry, PlaceRectInLotStaysInside)
{
    Polygon2D lot = MakeSquare(0.0f, 0.0f, 4.0f, 4.0f);
    Polygon2D rect = PlaceRectInLot(lot, 0.5f, 0.0f);
    for(const auto& v : rect.v){
        EXPECT_TRUE(PointInConvexCCW(lot, v));
    }
}

TEST(Geometry, MakeLShapeFootprintProducesCCW)
{
    Polygon2D rect = MakeSquare(0.0f, 0.0f, 4.0f, 4.0f);
    Polygon2D lshape = MakeLShapeFootprint(rect, 1.0f, 1.0f);
    ASSERT_EQ(lshape.v.size(), 6u);
    EXPECT_GT(SignedArea(lshape.v), 0.0f);
}

TEST(Geometry, ScaleFromCentroidKeepsCenter)
{
    Polygon2D square = MakeSquare(-1.0f, -1.0f, 1.0f, 1.0f);
    Vec2 before = Centroid(square);
    Polygon2D scaled = ScaleFromCentroid(square, 1.5f);
    Vec2 after = Centroid(scaled);
    EXPECT_NEAR(before.x, after.x, 1e-5f);
    EXPECT_NEAR(before.y, after.y, 1e-5f);
}

TEST(Geometry, OutsetFromCentroidGrowsFootprint)
{
    Polygon2D square = MakeSquare(-1.0f, -1.0f, 1.0f, 1.0f);
    Vec2 c = Centroid(square);
    float before = Length(square.v[0] - c);
    Polygon2D out = OutsetFromCentroid(square, 1.0f);
    float after = Length(out.v[0] - c);
    EXPECT_GT(after, before);
}

TEST(Geometry, IntersectConvexProducesExpectedArea)
{
    Polygon2D a = MakeSquare(0.0f, 0.0f, 2.0f, 2.0f);
    Polygon2D b = MakeSquare(1.0f, 1.0f, 3.0f, 3.0f);
    Polygon2D inter = IntersectConvex(a, b);
    EXPECT_NEAR(PolygonAreaAbs(inter), 1.0f, 1e-4f);
}

TEST(Geometry, UnionConvexHullCoversBoth)
{
    Polygon2D a = MakeSquare(0.0f, 0.0f, 2.0f, 2.0f);
    Polygon2D b = MakeSquare(1.0f, 1.0f, 3.0f, 3.0f);
    Polygon2D hull = UnionConvexHull(a, b);
    EXPECT_NEAR(PolygonAreaAbs(hull), 8.0f, 1e-4f);
}
