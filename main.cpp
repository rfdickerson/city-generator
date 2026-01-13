#include <vector>
#include <fstream>
#include <cmath>
#include <iostream>
#include <limits>
#include <cstdint>
#include <string>
#include <algorithm>

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
float Cross(Vec2 a, Vec2 b){ return a.x*b.y - a.y*b.x; }

// ============================================================
// Polygon (convex, CCW)
// ============================================================

struct Polygon2D {
    std::vector<Vec2> v;

    Polygon2D Inset(float d) const {
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
};

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

Polygon2D PlaceRectInLot(const Polygon2D& lot,
                         float shrink,
                         float snapStep)
{
    OBB2D obb = ComputeOBB(lot);
    Vec2 center = Centroid(lot);

    float halfX = std::max(0.0f, obb.halfX - shrink);
    float halfY = std::max(0.0f, obb.halfY - shrink);

    if(snapStep > 0.0f){
        halfX = std::floor(halfX / snapStep) * snapStep;
        halfY = std::floor(halfY / snapStep) * snapStep;
    }

    auto cornersInside = [&](float hx, float hy){
        Vec2 ax = obb.axisX * hx;
        Vec2 ay = obb.axisY * hy;
        Vec2 c0 = center + ax + ay;
        Vec2 c1 = center + ax - ay;
        Vec2 c2 = center - ax - ay;
        Vec2 c3 = center - ax + ay;
        return PointInConvexCCW(lot, c0) &&
               PointInConvexCCW(lot, c1) &&
               PointInConvexCCW(lot, c2) &&
               PointInConvexCCW(lot, c3);
    };

    float step = snapStep > 0.0f ? snapStep : 0.25f;
    int guard = 0;
    while(guard++ < 200 && !cornersInside(halfX, halfY)){
        halfX = std::max(0.0f, halfX - step);
        halfY = std::max(0.0f, halfY - step);
        if(halfX <= 0.0f || halfY <= 0.0f){
            break;
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
// glTF Writer (positions + uvs + vertex colors)
// ============================================================

static void AppendAligned(std::vector<std::uint8_t>& buf)
{
    while(buf.size() % 4 != 0){
        buf.push_back(0);
    }
}

template <typename T>
static size_t AppendData(std::vector<std::uint8_t>& buf, const T* data, size_t count)
{
    AppendAligned(buf);
    size_t offset = buf.size();
    const std::uint8_t* bytes = reinterpret_cast<const std::uint8_t*>(data);
    buf.insert(buf.end(), bytes, bytes + sizeof(T) * count);
    return offset;
}

static std::string ReplaceExtension(const std::string& path, const char* ext)
{
    size_t dot = path.find_last_of('.');
    if(dot == std::string::npos){
        return path + ext;
    }
    return path.substr(0, dot) + ext;
}

void WriteGLTF(const char* path, const Mesh& m)
{
    std::string gltfPath(path);
    std::string binPath = ReplaceExtension(gltfPath, ".bin");

    size_t vCount = m.v.size();
    size_t iCount = m.i.size();

    std::vector<float> positions;
    std::vector<float> colors;
    std::vector<float> uvs;
    positions.reserve(vCount * 3);
    colors.reserve(vCount * 4);
    uvs.reserve(vCount * 2);

    Vec3 posMin{std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max()};
    Vec3 posMax{-std::numeric_limits<float>::max(),
                -std::numeric_limits<float>::max(),
                -std::numeric_limits<float>::max()};

    for(const auto& v : m.v){
        positions.push_back(v.pos.x);
        positions.push_back(v.pos.y);
        positions.push_back(v.pos.z);

        colors.push_back(v.color.r);
        colors.push_back(v.color.g);
        colors.push_back(v.color.b);
        colors.push_back(v.color.a);

        uvs.push_back(v.uv.x);
        uvs.push_back(v.uv.y);

        posMin.x = std::min(posMin.x, v.pos.x);
        posMin.y = std::min(posMin.y, v.pos.y);
        posMin.z = std::min(posMin.z, v.pos.z);
        posMax.x = std::max(posMax.x, v.pos.x);
        posMax.y = std::max(posMax.y, v.pos.y);
        posMax.z = std::max(posMax.z, v.pos.z);
    }

    std::vector<std::uint8_t> buffer;
    size_t posOffset = AppendData(buffer, positions.data(), positions.size());
    size_t posLength = positions.size() * sizeof(float);

    size_t colorOffset = AppendData(buffer, colors.data(), colors.size());
    size_t colorLength = colors.size() * sizeof(float);

    size_t uvOffset = AppendData(buffer, uvs.data(), uvs.size());
    size_t uvLength = uvs.size() * sizeof(float);

    size_t idxOffset = AppendData(buffer, m.i.data(), m.i.size());
    size_t idxLength = m.i.size() * sizeof(unsigned);

    std::ofstream bout(binPath, std::ios::binary);
    bout.write(reinterpret_cast<const char*>(buffer.data()), (std::streamsize)buffer.size());

    std::ofstream jout(gltfPath);
    jout <<
        "{\n"
        "  \"asset\": {\"version\": \"2.0\"},\n"
        "  \"buffers\": [\n"
        "    {\"uri\": \"" << binPath.substr(binPath.find_last_of("/\\") + 1)
        << "\", \"byteLength\": " << buffer.size() << "}\n"
        "  ],\n"
        "  \"bufferViews\": [\n"
        "    {\"buffer\": 0, \"byteOffset\": " << posOffset << ", \"byteLength\": " << posLength << ", \"target\": 34962},\n"
        "    {\"buffer\": 0, \"byteOffset\": " << colorOffset << ", \"byteLength\": " << colorLength << ", \"target\": 34962},\n"
        "    {\"buffer\": 0, \"byteOffset\": " << uvOffset << ", \"byteLength\": " << uvLength << ", \"target\": 34962},\n"
        "    {\"buffer\": 0, \"byteOffset\": " << idxOffset << ", \"byteLength\": " << idxLength << ", \"target\": 34963}\n"
        "  ],\n"
        "  \"accessors\": [\n"
        "    {\"bufferView\": 0, \"componentType\": 5126, \"count\": " << vCount
        << ", \"type\": \"VEC3\", \"min\": [" << posMin.x << ", " << posMin.y << ", " << posMin.z
        << "], \"max\": [" << posMax.x << ", " << posMax.y << ", " << posMax.z << "]},\n"
        "    {\"bufferView\": 1, \"componentType\": 5126, \"count\": " << vCount
        << ", \"type\": \"VEC4\"},\n"
        "    {\"bufferView\": 2, \"componentType\": 5126, \"count\": " << vCount
        << ", \"type\": \"VEC2\"},\n"
        "    {\"bufferView\": 3, \"componentType\": 5125, \"count\": " << iCount
        << ", \"type\": \"SCALAR\", \"min\": [0], \"max\": [" << (iCount ? (unsigned)(*std::max_element(m.i.begin(), m.i.end())) : 0u) << "]}\n"
        "  ],\n"
        "  \"meshes\": [\n"
        "    {\"primitives\": [\n"
        "      {\"attributes\": {\"POSITION\": 0, \"COLOR_0\": 1, \"TEXCOORD_0\": 2}, \"indices\": 3}\n"
        "    ]}\n"
        "  ],\n"
        "  \"nodes\": [\n"
        "    {\"mesh\": 0}\n"
        "  ],\n"
        "  \"scenes\": [\n"
        "    {\"nodes\": [0]}\n"
        "  ],\n"
        "  \"scene\": 0\n"
        "}\n";
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

    // --- Building footprint from lot (OBB + shrink + snap + clamp) ---
    Polygon2D base = PlaceRectInLot(lot, 5.0f, 0.5f);

    // --- Stylized office parameters ---
    const int   floors       = 5;
    const float floorH       = 3.6f;
    const float slabT        = 0.75f;  // chunky SimCity slabs
    const float glassInset   = 1.2f;

    // --- Palette (cheery, graphic) ---
    Vec3 concrete = {0.55f,0.56f,0.57f};
    Vec3 window   = {0.10f,0.65f,0.95f};
    Vec3 roofDeck = {0.62f,0.62f,0.64f};
    Vec3 lotFill  = {0.25f,0.50f,0.30f};

    Mesh building;
    unsigned offset=0;

    // --- Lot visualization slab ---
    {
        Mesh lotMesh = BuildSlab({lot,-0.25f,0.25f},lotFill,0.03f);
        for(auto& i:lotMesh.i) i+=offset;
        building.v.insert(building.v.end(),lotMesh.v.begin(),lotMesh.v.end());
        building.i.insert(building.i.end(),lotMesh.i.begin(),lotMesh.i.end());
        offset += (unsigned)lotMesh.v.size();
    }

    float totalHeight = floors * floorH;

    for(int f=0;f<floors;f++){
        Polygon2D fp = base;

        float z = f * floorH;

        // --- slab ---
        Vec3 slabColor = (f == floors-1) ? roofDeck : concrete;
        Mesh slab = BuildSlab({fp,z,slabT},slabColor,0.02f);
        for(auto& i:slab.i) i+=offset;
        building.v.insert(building.v.end(),slab.v.begin(),slab.v.end());
        building.i.insert(building.i.end(),slab.i.begin(),slab.i.end());
        offset += (unsigned)slab.v.size();

        // --- curtain wall every 3 floors ---
        if(f % 3 == 0){
            float cwTop = std::min(z + 3 * floorH, totalHeight);
            if(cwTop <= z + slabT){
                continue;
            }
            Mesh cw = BuildCurtainWall(fp,
                                       z+slabT,
                                       cwTop,
                                       glassInset,
                                       window,
                                       concrete,
                                       2.2f,
                                       0.35f,
                                       0.15f);
            for(auto& i:cw.i) i+=offset;
            building.v.insert(building.v.end(),cw.v.begin(),cw.v.end());
            building.i.insert(building.i.end(),cw.i.begin(),cw.i.end());
            offset += (unsigned)cw.v.size();
        }
    }

    // --- Roof deck ---
    {
        Polygon2D roof = base.Inset(1.0f);
        float roofZ = totalHeight;
        float roofT = 0.25f;
        Mesh roofMesh = BuildSlab({roof,roofZ,roofT},roofDeck,0.02f);
        for(auto& i:roofMesh.i) i+=offset;
        building.v.insert(building.v.end(),roofMesh.v.begin(),roofMesh.v.end());
        building.i.insert(building.i.end(),roofMesh.i.begin(),roofMesh.i.end());
        offset += (unsigned)roofMesh.v.size();
    }

    WriteOBJ("simcity_midcentury_office.obj",building);
    WriteGLTF("simcity_midcentury_office.gltf",building);
    std::cout<<"Wrote simcity_midcentury_office.obj and simcity_midcentury_office.gltf\n";
}
