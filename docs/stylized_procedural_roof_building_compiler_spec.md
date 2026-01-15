# Stylized Procedural Building & Roof Compiler (Codex Spec)

This document defines a **two‑phase procedural architecture pipeline** for generating stylized, SimCity‑2013‑style diorama buildings. It is intended to be consumed by Codex or other code‑generation agents to implement deterministic, LOD‑friendly geometry generation in C++.

---

## 1. Design Goals

**Primary goals**
- Strong readability at multiple LODs (street → city → map view)
- Stylized, “toy / diorama” appearance
- Deterministic generation (no stochastic geometry)
- Clean export to **glTF** with:
  - Positions
  - Normals
  - UVs
  - Vertex colors
  - Ambient occlusion baked into **vertex color alpha**

**Non‑goals**
- Photorealism
- Runtime boolean geometry
- Half‑edge / CAD‑grade topology

---

## 2. High‑Level Architecture

The system is intentionally split into **two phases**.

```
PHASE 1: Conceptual Model
    Semantic volumes, footprints, parameters

PHASE 2: Geometry Compilation
    Vertices, indices, normals, UVs, vertex colors
```

This separation allows:
- Elegant extrusion logic
- Simple LOD collapse
- Stable AO baking
- Easy debugging and visualization

---

## 3. Core Data Types (Phase 1)

### 3.1 2D Geometry

```cpp
struct Vec2 { float x, y; };
struct Vec3 { float x, y, z; };

struct Polygon2D {
    std::vector<Vec2> vertices; // CCW, simple polygon
};
```

All polygons **must**:
- Be counter‑clockwise (CCW)
- Have no self‑intersections
- Have colinear points removed

---

### 3.2 Conceptual Volumes

```cpp
enum class VolumeType {
    Wall,
    Roof,
    Porch,
    Tower,
    Spire
};

struct Stylization {
    float corner_radius;     // silhouette softness
    float roof_exaggeration; // 1.2–1.4 typical
    float ao_strength;       // 0..1
};

struct Volume {
    Polygon2D footprint;
    float base_z;
    float height;
    VolumeType type;
    MaterialID material;
    Stylization style;
};
```

Volumes describe **intent**, not geometry.

---

### 3.3 Building Model

```cpp
struct BuildingModel {
    std::vector<Volume> volumes;
    BuildingMetadata metadata;
};
```

Examples:
- Bungalow → wall volume + roof volume + porch volume
- Church → nave + transept + tower + roof + spire volumes

---

## 4. Geometry Compilation (Phase 2)

### 4.1 MeshBuilder

All geometry is emitted through a single builder.

```cpp
struct MeshBuilder {
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec2> uvs;
    std::vector<Vec4> colors; // RGBA, alpha = AO
    std::vector<uint32_t> indices;
};
```

Rules:
- **Do not share vertices across faces** (flat shading)
- Emit normals per triangle
- Vertex colors always present

---

## 5. Roof Compiler Overview

Roofs are compiled using a **height‑function approach**:

```
2D roof footprint
 → triangulate
 → lift vertices using RoofHeight(x, y)
 → emit flat‑shaded triangles
```

This avoids complex roof topology and works on rectangles and L‑shapes.

---

## 6. Triangulation: Ear Clipping

### 6.1 Concept

Ear clipping triangulates a simple polygon by repeatedly removing “ears”:
- A triangle formed by three consecutive vertices
- The corner is convex
- No other polygon vertex lies inside the triangle

Time complexity: **O(n²)** — acceptable for building footprints.

### 6.2 Usage

```cpp
std::vector<uint32_t> TriangulateEarClipping(const std::vector<Vec2>& verts);
```

Triangulation occurs **before** roof height evaluation.

---

## 7. Roof Parameters

```cpp
enum class RoofType { Gable, Hip };

struct RoofParams {
    RoofType type;
    float pitch_deg;        // gable: ~22–32, hip: ~28–40
    float overhang;         // ~0.45m stylized
    float ridge_height;     // ~2.0m above wall top
    float hip_ridge_frac;   // 0..1 (hip roofs)
    float ao_strength;      // 0..1
};
```

---

## 8. Roof Local Frame

Roofs are evaluated in a **local 2D frame**:

```cpp
struct Frame2D {
    Vec2 origin;
    Vec2 axis_u; // ridge direction
    Vec2 axis_v; // perpendicular
};
```

Heuristic:
- `axis_u` aligns with the **longest footprint extent**
- Suitable for stylized roofs

---

## 9. Height Functions

### 9.1 Gable Roof

```cpp
float HeightGable(float v_abs, float ridge_h, float slope) {
    return max(0, ridge_h - slope * v_abs);
}
```

Produces a simple two‑plane roof.

---

### 9.2 Hip Roof (Stylized)

```cpp
float HeightHip(float u_abs, float v_abs,
                float ridge_half_len,
                float ridge_h,
                float slope) {
    float du = max(0, u_abs - ridge_half_len);
    float d  = max(v_abs, du);
    return max(0, ridge_h - slope * d);
}
```

Uses an L‑infinity distance for crisp, chunky facets.

---

## 10. Stylized AO Baking (Vertex Alpha)

Ambient occlusion is **rule‑based**, not ray‑traced.

### 10.1 Inputs
- Height relative to roof
- Distance to roof edge
- Overhang presence

### 10.2 AO Convention

```cpp
VertexColor {
    rgb = base_color;
    a   = ambient_occlusion; // 0..1
}
```

### 10.3 Roof AO Rule (Example)

```cpp
float ao = 0.70f + 0.30f * edgeDistance01;
ao = Lerp(1.0f, ao, ao_strength);
```

AO is intentionally subtle (never stronger than ~35%).

---

## 11. Stylization Rules (Critical)

- Large planar faces only
- Overhangs exaggerated
- No micro‑detail geometry
- Flat colors + AO do the visual work

If geometry looks good up close but noisy from far away → reject it.

---

## 12. LOD Strategy (By Construction)

| LOD | Strategy |
|----|---------|
| 0 | Full mesh, AO enabled |
| 1 | Merge coplanar faces |
| 2 | Replace with prism |
| 3 | Billboard / proxy |

Because AO is baked per vertex, no lighting breaks occur.

---

## 13. Export Expectations

Final output mesh must be compatible with **glTF 2.0**:

- `POSITION`
- `NORMAL`
- `TEXCOORD_0`
- `COLOR_0` (RGBA, alpha = AO)
- Indexed triangles

Blender will import:
- Vertex colors as a Color Attribute
- UVs as UVMap

---

## 14. Mental Model (Summary)

> Procedural architecture is **compilation**, not modeling.

Describe intent first.
Compile geometry second.
Stylize aggressively.
Optimize for readability.

This document defines the contract Codex should follow when generating or extending the system.

