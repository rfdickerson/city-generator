#include "geometry.h"

#include <algorithm>
#include <limits>
#include <cmath>

Polygon2D Polygon2D::Inset(float d) const
{
    int n = (int)v.size();
    std::vector<Vec2> out(n);

    for(int i=0;i<n;i++){
        Vec2 pPrev = v[(i-1+n)%n];
        Vec2 pCurr = v[i];
        Vec2 pNext = v[(i+1)%n];

        Vec2 e0 = Normalize({pCurr.x-pPrev.x, pCurr.y-pPrev.y});
        Vec2 e1 = Normalize({pNext.x-pCurr.x, pNext.y-pCurr.y});

        // CCW polygon: left normals point inward.
        Vec2 n0 = Perp(e0);
        Vec2 n1 = Perp(e1);

        Vec2 l0p = pCurr + n0*d;
        Vec2 l1p = pCurr + n1*d;

        float denom = Cross(e0, e1);
        if(std::fabs(denom) < 1e-6f){
            out[i] = l0p;
            continue;
        }

        float t = Cross({l1p.x-l0p.x, l1p.y-l0p.y}, e1) / denom;
        out[i] = {l0p.x + e0.x*t, l0p.y + e0.y*t};
    }
    return {out};
}

Vec2 Centroid(const Polygon2D& p)
{
    Vec2 c{0,0};
    for(const auto& v : p.v){
        c = c + v;
    }
    float inv = 1.0f / (float)p.v.size();
    return c * inv;
}

bool PointInConvexCCW(const Polygon2D& poly, Vec2 p)
{
    int n = (int)poly.v.size();
    for(int i=0;i<n;i++){
        Vec2 a = poly.v[i];
        Vec2 b = poly.v[(i+1)%n];
        Vec2 ab = {b.x-a.x, b.y-a.y};
        Vec2 ap = {p.x-a.x, p.y-a.y};
        if(Cross(ab, ap) < 0.0f){
            return false;
        }
    }
    return true;
}

float SignedArea(const std::vector<Vec2>& v)
{
    float a = 0.0f;
    int n = (int)v.size();
    for(int i=0;i<n;i++){
        const Vec2& p = v[i];
        const Vec2& q = v[(i+1)%n];
        a += p.x * q.y - q.x * p.y;
    }
    return 0.5f * a;
}

bool PointInPolygon(const Polygon2D& poly, Vec2 p)
{
    bool inside = false;
    size_t n = poly.v.size();
    for(size_t i=0, j=n-1; i<n; j=i++){
        Vec2 vi = poly.v[i];
        Vec2 vj = poly.v[j];
        bool intersect = ((vi.y > p.y) != (vj.y > p.y)) &&
                         (p.x < (vj.x - vi.x) * (p.y - vi.y) / (vj.y - vi.y + 1e-12f) + vi.x);
        if(intersect){
            inside = !inside;
        }
    }
    return inside;
}
static bool PointInTri(Vec2 p, Vec2 a, Vec2 b, Vec2 c, float sign, float eps)
{
    float c0 = Cross({b.x-a.x, b.y-a.y}, {p.x-a.x, p.y-a.y}) * sign;
    float c1 = Cross({c.x-b.x, c.y-b.y}, {p.x-b.x, p.y-b.y}) * sign;
    float c2 = Cross({a.x-c.x, a.y-c.y}, {p.x-c.x, p.y-c.y}) * sign;
    return c0 >= -eps && c1 >= -eps && c2 >= -eps;
}

std::vector<unsigned> TriangulateCCW(const std::vector<Vec2>& v)
{
    std::vector<unsigned> out;
    int n = (int)v.size();
    if(n < 3){
        return out;
    }

    auto clip = [&](const std::vector<Vec2>& pts, bool reversed){
        std::vector<unsigned> tris;
        std::vector<int> idx(pts.size());
        for(size_t i=0;i<pts.size();i++) idx[i]=(int)i;

        float sign = (SignedArea(pts) >= 0.0f) ? 1.0f : -1.0f;
        const float eps = 1e-6f;
        int guard = 0;
        while(idx.size() > 2 && guard++ < (int)pts.size()*(int)pts.size()){
            bool clipped = false;
            int m = (int)idx.size();
            for(int i=0;i<m;i++){
                int i0 = idx[(i-1+m)%m];
                int i1 = idx[i];
                int i2 = idx[(i+1)%m];

                Vec2 a = pts[i0];
                Vec2 b = pts[i1];
                Vec2 c = pts[i2];

                if(Cross({b.x-a.x, b.y-a.y}, {c.x-a.x, c.y-a.y}) * sign <= eps){
                    continue;
                }

                bool hasInside = false;
                for(int j=0;j<m;j++){
                    int vi = idx[j];
                    if(vi==i0 || vi==i1 || vi==i2) continue;
                    if(PointInTri(pts[vi], a, b, c, sign, eps)){
                        hasInside = true;
                        break;
                    }
                }
                if(hasInside) continue;

                unsigned aidx = (unsigned)i0;
                unsigned bidx = (unsigned)i1;
                unsigned cidx = (unsigned)i2;
                if(reversed){
                    aidx = (unsigned)(pts.size() - 1 - i0);
                    bidx = (unsigned)(pts.size() - 1 - i1);
                    cidx = (unsigned)(pts.size() - 1 - i2);
                }
                tris.push_back(aidx);
                tris.push_back(bidx);
                tris.push_back(cidx);
                idx.erase(idx.begin()+i);
                clipped = true;
                break;
            }
            if(!clipped){
                break;
            }
        }
        return tris;
    };

    out = clip(v, false);
    if(out.size() != (size_t)(n-2)*3){
        std::vector<Vec2> rev = v;
        std::reverse(rev.begin(), rev.end());
        std::vector<unsigned> alt = clip(rev, true);
        if(alt.size() > out.size()){
            out = alt;
        }
    }
    return out;
}

OBB2D ComputeOBB(const Polygon2D& poly)
{
    int n = (int)poly.v.size();
    if(n < 2){
        return {{0,0},{1,0},{0,1},0,0};
    }

    float bestArea = std::numeric_limits<float>::max();
    OBB2D best = {{0,0},{1,0},{0,1},0,0};

    for(int i=0;i<n;i++){
        Vec2 p0 = poly.v[i];
        Vec2 p1 = poly.v[(i+1)%n];
        Vec2 edge = Normalize({p1.x-p0.x, p1.y-p0.y});
        Vec2 axisX = edge;
        Vec2 axisY = Perp(axisX);

        float minU = std::numeric_limits<float>::max();
        float maxU = -std::numeric_limits<float>::max();
        float minV = std::numeric_limits<float>::max();
        float maxV = -std::numeric_limits<float>::max();

        for(const auto& p : poly.v){
            float u = Dot(p, axisX);
            float v = Dot(p, axisY);
            minU = std::min(minU, u);
            maxU = std::max(maxU, u);
            minV = std::min(minV, v);
            maxV = std::max(maxV, v);
        }

        float area = (maxU - minU) * (maxV - minV);
        if(area < bestArea){
            bestArea = area;
            float midU = (minU + maxU) * 0.5f;
            float midV = (minV + maxV) * 0.5f;
            Vec2 center = axisX * midU + axisY * midV;
            best = {center, axisX, axisY, (maxU - minU) * 0.5f, (maxV - minV) * 0.5f};
        }
    }

    return best;
}

Polygon2D PlaceRectInLot(const Polygon2D& lot, float shrink, float snapStep, Vec2 biasDir, float biasStrength)
{
    OBB2D obb = ComputeOBB(lot);
    Vec2 center = Centroid(lot);

    float shrinkX = shrink;
    float shrinkY = shrink;
    if(obb.halfX > 1e-4f && obb.halfY > 1e-4f){
        float aspect = std::max(obb.halfX, obb.halfY) / std::min(obb.halfX, obb.halfY);
        if(aspect > 1.35f){
            // Keep elongated lots feeling long by relaxing the setback on the long axis.
            if(obb.halfX >= obb.halfY){
                shrinkX *= 0.5f;
            }else{
                shrinkY *= 0.5f;
            }
        }
    }

    float halfX = std::max(0.0f, obb.halfX - shrinkX);
    float halfY = std::max(0.0f, obb.halfY - shrinkY);

    if(snapStep > 0.0f){
        halfX = std::floor(halfX / snapStep) * snapStep;
        halfY = std::floor(halfY / snapStep) * snapStep;
    }

    auto cornersInside = [&](float hx, float hy, Vec2 c){
        Vec2 ax = obb.axisX * hx;
        Vec2 ay = obb.axisY * hy;
        Vec2 c0 = c + ax + ay;
        Vec2 c1 = c + ax - ay;
        Vec2 c2 = c - ax - ay;
        Vec2 c3 = c - ax + ay;
        return PointInConvexCCW(lot, c0) &&
               PointInConvexCCW(lot, c1) &&
               PointInConvexCCW(lot, c2) &&
               PointInConvexCCW(lot, c3);
    };

    float step = snapStep > 0.0f ? snapStep : 0.25f;
    int guard = 0;
    while(guard++ < 200 && !cornersInside(halfX, halfY, center)){
        halfX = std::max(0.0f, halfX - step);
        halfY = std::max(0.0f, halfY - step);
        if(halfX <= 0.0f || halfY <= 0.0f){
            break;
        }
    }

    float biasLen = Length(biasDir);
    if(biasLen > 1e-4f && biasStrength > 0.0f){
        Vec2 dir = {biasDir.x / biasLen, biasDir.y / biasLen};
        float slackX = std::max(0.0f, obb.halfX - halfX);
        float slackY = std::max(0.0f, obb.halfY - halfY);
        float maxShift = std::numeric_limits<float>::max();
        float dx = Dot(dir, obb.axisX);
        float dy = Dot(dir, obb.axisY);
        const float eps = 1e-4f;
        if(std::fabs(dx) > eps){
            maxShift = std::min(maxShift, slackX / std::fabs(dx));
        }
        if(std::fabs(dy) > eps){
            maxShift = std::min(maxShift, slackY / std::fabs(dy));
        }
        if(maxShift != std::numeric_limits<float>::max()){
            float strength = std::max(0.0f, std::min(1.0f, biasStrength));
            float shift = strength * maxShift;
            Vec2 biasedCenter = center + dir * shift;
            int biasGuard = 0;
            while(biasGuard++ < 20 && !cornersInside(halfX, halfY, biasedCenter)){
                shift *= 0.5f;
                biasedCenter = center + dir * shift;
            }
            center = biasedCenter;
        }
    }

    Vec2 ax = obb.axisX * halfX;
    Vec2 ay = obb.axisY * halfY;
    // CCW rectangle so inset operations move inward.
    Polygon2D rect {{
        center + ax + ay,
        center - ax + ay,
        center - ax - ay,
        center + ax - ay
    }};

    return rect;
}

Polygon2D PlaceRectInLot(const Polygon2D& lot, float shrink, float snapStep)
{
    return PlaceRectInLot(lot, shrink, snapStep, {0.0f, 0.0f}, 0.0f);
}

Polygon2D EnforceRectAspect(const Polygon2D& rect, float targetAspect, float minShortHalf)
{
    if(targetAspect <= 1.01f || rect.v.size() < 4){
        return rect;
    }
    Vec2 center = Centroid(rect);
    Vec2 axisX = Normalize({rect.v[0].x-rect.v[1].x, rect.v[0].y-rect.v[1].y});
    Vec2 axisY = Normalize({rect.v[1].x-rect.v[2].x, rect.v[1].y-rect.v[2].y});
    float halfX = 0.5f * Length({rect.v[0].x-rect.v[1].x, rect.v[0].y-rect.v[1].y});
    float halfY = 0.5f * Length({rect.v[1].x-rect.v[2].x, rect.v[1].y-rect.v[2].y});
    bool longIsX = halfX >= halfY;
    float longHalf = longIsX ? halfX : halfY;
    float shortHalf = longIsX ? halfY : halfX;
    float targetShort = longHalf / targetAspect;
    float newShort = std::max(minShortHalf, targetShort);
    if(newShort >= shortHalf){
        return rect;
    }
    if(longIsX){
        halfY = newShort;
    }else{
        halfX = newShort;
    }
    Vec2 ax = axisX * halfX;
    Vec2 ay = axisY * halfY;
    return Polygon2D {{
        center + ax + ay,
        center - ax + ay,
        center - ax - ay,
        center + ax - ay
    }};
}

Polygon2D MakeLShapeFootprint(const Polygon2D& baseRect, float cutX, float cutY)
{
    Vec2 center = Centroid(baseRect);
    Vec2 axisX = Normalize({baseRect.v[0].x-baseRect.v[1].x, baseRect.v[0].y-baseRect.v[1].y});
    Vec2 axisY = Normalize({baseRect.v[1].x-baseRect.v[2].x, baseRect.v[1].y-baseRect.v[2].y});
    float halfX = 0.5f * Length({baseRect.v[0].x-baseRect.v[1].x, baseRect.v[0].y-baseRect.v[1].y});
    float halfY = 0.5f * Length({baseRect.v[1].x-baseRect.v[2].x, baseRect.v[1].y-baseRect.v[2].y});

    float cx = std::min(std::max(cutX, 0.0f), halfX - 0.5f);
    float cy = std::min(std::max(cutY, 0.0f), halfY - 0.5f);

    auto P = [&](float u, float v){
        return center + axisX * u + axisY * v;
    };

    Polygon2D l {{
        P(+halfX, +halfY - cy),
        P(+halfX, -halfY),
        P(-halfX, -halfY),
        P(-halfX, +halfY),
        P(+halfX - cx, +halfY),
        P(+halfX - cx, +halfY - cy)
    }};
    if(SignedArea(l.v) < 0.0f){
        std::reverse(l.v.begin(), l.v.end());
    }
    return l;
}

Polygon2D ScaleFromCentroid(const Polygon2D& p, float scale)
{
    Vec2 c = Centroid(p);
    Polygon2D out = p;
    for(auto& v : out.v){
        v = c + (v - c) * scale;
    }
    return out;
}

Polygon2D OutsetFromCentroid(const Polygon2D& p, float delta)
{
    Vec2 c = Centroid(p);
    float avg = 0.0f;
    for(const auto& v : p.v){
        avg += Length(v - c);
    }
    avg = (p.v.empty()) ? 0.0f : avg / (float)p.v.size();
    if(avg <= 1e-4f){
        return p;
    }
    float scale = (avg + delta) / avg;
    if(scale < 0.05f){
        scale = 0.05f;
    }
    return ScaleFromCentroid(p, scale);
}

Polygon2D IntersectConvex(const Polygon2D& subject, const Polygon2D& clipper)
{
    Polygon2D out = subject;
    int clipCount = (int)clipper.v.size();
    for(int i=0;i<clipCount;i++){
        Vec2 a = clipper.v[i];
        Vec2 b = clipper.v[(i+1)%clipCount];
        std::vector<Vec2> input = out.v;
        out.v.clear();
        if(input.empty()){
            break;
        }

        Vec2 edge = {b.x-a.x, b.y-a.y};
        for(size_t j=0;j<input.size();j++){
            Vec2 p = input[j];
            Vec2 q = input[(j+1)%input.size()];
            float cp = Cross(edge, {p.x-a.x, p.y-a.y});
            float cq = Cross(edge, {q.x-a.x, q.y-a.y});
            bool pin = cp >= 0.0f;
            bool qin = cq >= 0.0f;

            if(pin && qin){
                out.v.push_back(q);
            }else if(pin && !qin){
                float t = cp / (cp - cq);
                out.v.push_back({p.x + (q.x-p.x)*t, p.y + (q.y-p.y)*t});
            }else if(!pin && qin){
                float t = cp / (cp - cq);
                out.v.push_back({p.x + (q.x-p.x)*t, p.y + (q.y-p.y)*t});
                out.v.push_back(q);
            }
        }
    }
    return out;
}

static std::vector<Vec2> ConvexHull(const std::vector<Vec2>& pts)
{
    std::vector<Vec2> p = pts;
    if(p.size() <= 1){
        return p;
    }
    std::sort(p.begin(), p.end(), [](const Vec2& a, const Vec2& b){
        if(a.x == b.x) return a.y < b.y;
        return a.x < b.x;
    });

    std::vector<Vec2> lower;
    for(const auto& v : p){
        while(lower.size() >= 2){
            Vec2 a = lower[lower.size()-2];
            Vec2 b = lower[lower.size()-1];
            if(Cross({b.x-a.x, b.y-a.y}, {v.x-b.x, v.y-b.y}) > 0.0f){
                break;
            }
            lower.pop_back();
        }
        lower.push_back(v);
    }

    std::vector<Vec2> upper;
    for(int i=(int)p.size()-1;i>=0;i--){
        Vec2 v = p[i];
        while(upper.size() >= 2){
            Vec2 a = upper[upper.size()-2];
            Vec2 b = upper[upper.size()-1];
            if(Cross({b.x-a.x, b.y-a.y}, {v.x-b.x, v.y-b.y}) > 0.0f){
                break;
            }
            upper.pop_back();
        }
        upper.push_back(v);
    }

    lower.pop_back();
    upper.pop_back();
    lower.insert(lower.end(), upper.begin(), upper.end());
    return lower;
}

Polygon2D UnionConvexHull(const Polygon2D& a, const Polygon2D& b)
{
    std::vector<Vec2> pts = a.v;
    pts.insert(pts.end(), b.v.begin(), b.v.end());
    return {ConvexHull(pts)};
}
