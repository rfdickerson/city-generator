#include "mesh.h"

#include <algorithm>

void AddQuad(Mesh& m, unsigned a, unsigned b, unsigned c, unsigned d)
{
    m.i.insert(m.i.end(), {a,b,c, a,c,d});
}

void AddBox(Mesh& m,
            Vec2 center,
            Vec2 axisX,
            Vec2 axisY,
            float halfX,
            float halfY,
            float z0,
            float z1,
            Vec3 color)
{
    Vec2 ax = axisX * halfX;
    Vec2 ay = axisY * halfY;

    Vec2 c0 = center + ax + ay;
    Vec2 c1 = center - ax + ay;
    Vec2 c2 = center - ax - ay;
    Vec2 c3 = center + ax - ay;

    unsigned base = (unsigned)m.v.size();
    Vec2 uv0{0,0};

    m.v.push_back({{c0.x,z0,c0.y},{color.x,color.y,color.z,1.0f},uv0});
    m.v.push_back({{c1.x,z0,c1.y},{color.x,color.y,color.z,1.0f},uv0});
    m.v.push_back({{c2.x,z0,c2.y},{color.x,color.y,color.z,1.0f},uv0});
    m.v.push_back({{c3.x,z0,c3.y},{color.x,color.y,color.z,1.0f},uv0});

    m.v.push_back({{c0.x,z1,c0.y},{color.x,color.y,color.z,1.0f},uv0});
    m.v.push_back({{c1.x,z1,c1.y},{color.x,color.y,color.z,1.0f},uv0});
    m.v.push_back({{c2.x,z1,c2.y},{color.x,color.y,color.z,1.0f},uv0});
    m.v.push_back({{c3.x,z1,c3.y},{color.x,color.y,color.z,1.0f},uv0});

    AddQuad(m,base+0,base+1,base+2,base+3); // bottom
    AddQuad(m,base+4,base+7,base+6,base+5); // top
    AddQuad(m,base+0,base+4,base+5,base+1);
    AddQuad(m,base+1,base+5,base+6,base+2);
    AddQuad(m,base+2,base+6,base+7,base+3);
    AddQuad(m,base+3,base+7,base+4,base+0);
}

Mesh BuildSlab(const FloorSlab& s, Vec3 baseColor, float uvScale)
{
    return BuildSlab(s, baseColor, uvScale, SlabRole::Office);
}

static float Clamp01(float v)
{
    return std::max(0.0f, std::min(1.0f, v));
}

static Vec3 Tint(Vec3 c, float mul)
{
    return {Clamp01(c.x * mul), Clamp01(c.y * mul), Clamp01(c.z * mul)};
}

struct RoleStyle {
    float colorMul;
    float aoTop;
    float aoBottom;
    float thicknessMul;
};

static RoleStyle StyleForRole(SlabRole role)
{
    switch(role){
    case SlabRole::Infrastructure:
        return {0.85f, 0.75f, 0.45f, 0.70f};
    case SlabRole::Podium:
        return {1.00f, 1.00f, 0.65f, 1.25f};
    case SlabRole::Public:
        return {0.95f, 0.90f, 0.55f, 0.85f};
    case SlabRole::Terrace:
        return {1.05f, 1.00f, 0.60f, 0.65f};
    case SlabRole::Roof:
        return {1.05f, 1.00f, 0.70f, 1.35f};
    case SlabRole::Office:
    default:
        return {1.00f, 1.00f, 0.60f, 1.00f};
    }
}

Mesh BuildSlab(const FloorSlab& s, Vec3 baseColor, float uvScale, SlabRole role)
{
    Mesh m;
    int n = (int)s.footprint.v.size();

    RoleStyle style = StyleForRole(role);
    Vec3 tint = Tint(baseColor, style.colorMul);
    float thickness = s.thickness * style.thicknessMul;

    float z0 = s.z;
    float z1 = s.z + thickness;

    // --- Bottom (underside, darker AO) ---
    for(auto& p : s.footprint.v){
        m.v.push_back({
            {p.x, z0, p.y},
            {tint.x, tint.y, tint.z, style.aoBottom},
            {p.x*uvScale, p.y*uvScale}
        });
    }

    // --- Top (lighter) ---
    for(auto& p : s.footprint.v){
        m.v.push_back({
            {p.x, z1, p.y},
            {tint.x, tint.y, tint.z, style.aoTop},
            {p.x*uvScale, p.y*uvScale}
        });
    }

    // caps (support concave footprints)
    std::vector<unsigned> tri = TriangulateCCW(s.footprint.v);
    for(size_t k=0;k<tri.size();k+=3){
        unsigned a = tri[k];
        unsigned b = tri[k+1];
        unsigned c = tri[k+2];
        Vec2 pa = s.footprint.v[a];
        Vec2 pb = s.footprint.v[b];
        Vec2 pc = s.footprint.v[c];
        float area2 = std::fabs(Cross({pb.x-pa.x, pb.y-pa.y}, {pc.x-pa.x, pc.y-pa.y}));
        if(area2 < 1e-6f){
            continue;
        }
        Vec2 centroid{(pa.x + pb.x + pc.x) / 3.0f, (pa.y + pb.y + pc.y) / 3.0f};
        if(!PointInPolygon(s.footprint, centroid)){
            continue;
        }
        m.i.insert(m.i.end(), {a,c,b}); // bottom
        m.i.insert(m.i.end(), {n+a,n+b,n+c}); // top
    }

    // sides (vertical gradient AO)
    for(int i=0;i<n;i++){
        int j=(i+1)%n;
        AddQuad(m,i,j,n+j,n+i);
    }

    return m;
}

Mesh BuildCurtainWall(const Polygon2D& fp,
                      float z0,float z1,
                      float inset,
                      Vec3 windowColor,
                      Vec3 mullionColor,
                      float panelWidth,
                      float mullionWidth,
                      float uvVScale)
{
    Mesh m;
    Polygon2D g = fp.Inset(inset);
    int n = (int)g.v.size();

    float ao = std::max(0.6f, 1.0f - inset*0.15f);

    for(int i=0;i<n;i++){
        int j=(i+1)%n;
        Vec2 a=g.v[i], b=g.v[j];
        Vec2 dir = Normalize({b.x-a.x, b.y-a.y});
        float len = Length({b.x-a.x, b.y-a.y});

        float t = 0.0f;
        float u0 = 0.0f;
        bool isWindow = true;

        while(t < len){
            float seg = isWindow ? panelWidth : mullionWidth;
            if(seg <= 0.0f){
                seg = len - t;
            }
            float segLen = std::min(seg, len - t);

            Vec2 p0 = {a.x + dir.x * t, a.y + dir.y * t};
            Vec2 p1 = {a.x + dir.x * (t + segLen), a.y + dir.y * (t + segLen)};

            Vec3 col = isWindow ? windowColor : mullionColor;
            unsigned base=(unsigned)m.v.size();

            m.v.push_back({{p0.x,z0,p0.y},{col.x,col.y,col.z,ao},{u0,0}});
            m.v.push_back({{p1.x,z0,p1.y},{col.x,col.y,col.z,ao},{u0+segLen,0}});
            m.v.push_back({{p1.x,z1,p1.y},{col.x,col.y,col.z,ao},{u0+segLen,(z1-z0)*uvVScale}});
            m.v.push_back({{p0.x,z1,p0.y},{col.x,col.y,col.z,ao},{u0,(z1-z0)*uvVScale}});

            AddQuad(m,base,base+1,base+2,base+3);

            t += segLen;
            u0 += segLen;
            if(isWindow && mullionWidth > 0.0f){
                isWindow = false;
            }else{
                isWindow = true;
            }
        }
    }
    return m;
}
