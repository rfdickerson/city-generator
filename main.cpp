#include <vector>
#include <fstream>
#include <cmath>
#include <iostream>
#include <limits>

// ============================================================
// Basic Math
// ============================================================

struct Vec2 { float x, y; };
struct Vec3 { float x, y, z; };
struct Vec4 { float r, g, b, a; };

Vec2 operator+(Vec2 a, Vec2 b){ return {a.x+b.x, a.y+b.y}; }
Vec2 operator-(Vec2 a, Vec2 b){ return {a.x-b.x, a.y-b.y}; }
Vec2 operator*(Vec2 a, float s){ return {a.x*s, a.y*s}; }

float Dot(Vec2 a, Vec2 b){ return a.x*b.x + a.y*b.y; }
float Length(Vec2 v){ return std::sqrt(v.x*v.x + v.y*v.y); }
Vec2 Normalize(Vec2 v){ float len = Length(v); return {v.x/len, v.y/len}; }
Vec2 Perp(Vec2 v){ return {-v.y, v.x}; }

// ============================================================
// Polygon (convex, CCW)
// ============================================================

struct Polygon2D {
    std::vector<Vec2> v;

    Polygon2D Inset(float d) const {
        int n = (int)v.size();
        std::vector<Vec2> out(n);

        for(int i=0;i<n;i++){
            Vec2 p0 = v[i];
            Vec2 p1 = v[(i+1)%n];
            Vec2 e  = {p1.x-p0.x, p1.y-p0.y};
            Vec2 nrm = {-e.y, e.x};
            float len = Length(nrm);
            nrm = {nrm.x/len, nrm.y/len};
            out[i] = p0 + nrm*d;
        }
        return {out};
    }
};

// ============================================================
// OBB (2D, minimal area)
// ============================================================

struct OBB2D {
    Vec2 center;
    Vec2 axisX;
    Vec2 axisY;
    float halfX;
    float halfY;
};

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

// ============================================================
// Mesh
// ============================================================

struct Vertex {
    Vec3 pos;
    Vec4 color; // RGB = base color, A = AO
    Vec2 uv;
};

struct Mesh {
    std::vector<Vertex> v;
    std::vector<unsigned> i;
};

void AddQuad(Mesh& m,
             unsigned a,unsigned b,
             unsigned c,unsigned d)
{
    m.i.insert(m.i.end(), {a,b,c, a,c,d});
}

// ============================================================
// Stylized Slab
// ============================================================

struct FloorSlab {
    Polygon2D footprint;
    float z;
    float thickness;
};

Mesh BuildSlab(const FloorSlab& s,
               Vec3 baseColor,
               float uvScale)
{
    Mesh m;
    int n = (int)s.footprint.v.size();

    float z0 = s.z;
    float z1 = s.z + s.thickness;

    // --- Bottom (underside, darker AO) ---
    for(auto& p : s.footprint.v){
        m.v.push_back({
            {p.x, z0, p.y},
            {baseColor.x, baseColor.y, baseColor.z, 0.60f},
            {p.x*uvScale, p.y*uvScale}
        });
    }

    // --- Top (lighter) ---
    for(auto& p : s.footprint.v){
        m.v.push_back({
            {p.x, z1, p.y},
            {baseColor.x, baseColor.y, baseColor.z, 1.00f},
            {p.x*uvScale, p.y*uvScale}
        });
    }

    // caps
    for(int i=1;i+1<n;i++){
        m.i.insert(m.i.end(), {0,(unsigned)(i+1),(unsigned)i});
        m.i.insert(m.i.end(), {(unsigned)n,(unsigned)(n+i),(unsigned)(n+i+1)});
    }

    // sides (vertical gradient AO)
    for(int i=0;i<n;i++){
        int j=(i+1)%n;
        AddQuad(m,i,j,n+j,n+i);
    }

    return m;
}

// ============================================================
// Curtain Wall (grouped floors, stylized)
// ============================================================

Mesh BuildCurtainWall(const Polygon2D& fp,
                      float z0,float z1,
                      float inset,
                      Vec3 glassColor,
                      float uvVScale)
{
    Mesh m;
    Polygon2D g = fp.Inset(inset);
    int n = (int)g.v.size();

    float ao = std::max(0.6f, 1.0f - inset*0.15f);

    for(int i=0;i<n;i++){
        int j=(i+1)%n;
        Vec2 a=g.v[i], b=g.v[j];
        unsigned base=(unsigned)m.v.size();

        m.v.push_back({{a.x,z0,a.y},{glassColor.x,glassColor.y,glassColor.z,ao},{0,0}});
        m.v.push_back({{b.x,z0,b.y},{glassColor.x,glassColor.y,glassColor.z,ao},{1,0}});
        m.v.push_back({{b.x,z1,b.y},{glassColor.x,glassColor.y,glassColor.z,ao},{1,(z1-z0)*uvVScale}});
        m.v.push_back({{a.x,z1,a.y},{glassColor.x,glassColor.y,glassColor.z,ao},{0,(z1-z0)*uvVScale}});

        AddQuad(m,base,base+1,base+2,base+3);
    }
    return m;
}

// ============================================================
// OBJ Writer (vertex color + AO)
// ============================================================

void WriteOBJ(const char* path,const Mesh& m)
{
    std::ofstream out(path);

    for(auto& v:m.v)
        out<<"v "<<v.pos.x<<" "<<v.pos.y<<" "<<v.pos.z<<"\n";

    for(auto& v:m.v)
        out<<"vt "<<v.uv.x<<" "<<v.uv.y<<"\n";

    for(auto& v:m.v)
        out<<"vc "<<v.color.r<<" "<<v.color.g<<" "<<v.color.b<<" "<<v.color.a<<"\n";

    for(size_t k=0;k<m.i.size();k+=3){
        unsigned a=m.i[k]+1,b=m.i[k+1]+1,c=m.i[k+2]+1;
        out<<"f "<<a<<"/"<<a<<" "<<b<<"/"<<b<<" "<<c<<"/"<<c<<"\n";
    }
}

// ============================================================
// Main
// ============================================================

int main()
{
    // --- Lot footprint (meters) ---
    Polygon2D lot {{
        {-18,-12},{18,-12},{22,10},{-14,14}
    }};

    // --- Base setback ---
    Polygon2D base = lot.Inset(3.0f);

    // --- Stylized office parameters ---
    const int   floors       = 12;
    const float floorH       = 4.6f;   // exaggerated
    const float slabT        = 0.75f;  // chunky SimCity slabs
    const float glassInset   = 1.2f;

    // --- Palette (cheery, graphic) ---
    Vec3 concrete = {0.55f,0.56f,0.57f};
    Vec3 glass    = {0.25f,0.45f,0.65f};

    Mesh building;
    unsigned offset=0;

    for(int f=0;f<floors;f++){
        Polygon2D fp = base;

        // exaggerated step-backs
        if(f>=6)  fp = fp.Inset(2.0f);
        if(f>=9)  fp = fp.Inset(1.5f);

        float z = f * floorH;

        // --- slab ---
        Mesh slab = BuildSlab({fp,z,slabT},concrete,0.02f);
        for(auto& i:slab.i) i+=offset;
        building.v.insert(building.v.end(),slab.v.begin(),slab.v.end());
        building.i.insert(building.i.end(),slab.i.begin(),slab.i.end());
        offset += (unsigned)slab.v.size();

        // --- curtain wall every 3 floors ---
        if(f % 3 == 0){
            Mesh cw = BuildCurtainWall(fp,
                                       z+slabT,
                                       z+3*floorH,
                                       glassInset,
                                       glass,
                                       0.15f);
            for(auto& i:cw.i) i+=offset;
            building.v.insert(building.v.end(),cw.v.begin(),cw.v.end());
            building.i.insert(building.i.end(),cw.i.begin(),cw.i.end());
            offset += (unsigned)cw.v.size();
        }
    }

    WriteOBJ("simcity_midcentury_office.obj",building);
    std::cout<<"Wrote simcity_midcentury_office.obj\n";
}
