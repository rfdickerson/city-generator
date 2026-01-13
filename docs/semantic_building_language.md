# Semantic Building Language (SBL)

## Purpose

This document defines the **Semantic Building Language (SBL)** used by this project.

SBL is a **semantic-first architectural model**: buildings are described by *meaning* and *intent* before any geometry is generated. Geometry, shading, props, and LOD are **derived consequences**, not primary inputs.

This document is intended to be consumed by:
- Human contributors
- Automated agents (e.g. Codex)
- Procedural generation systems

Agents **must not** generate meshes directly without first producing or modifying semantic descriptions defined here.

---

## Core Principle

> **No geometry is generated without semantic justification.**

Buildings are not meshes; they are **architectural statements** compiled into geometry.

---

## Semantic Layers Overview

The system is organized into the following semantic layers, evaluated in order:

1. Urban intent (why the building exists)
2. Site relationship (how it sits in the lot)
3. Massing grammar (how volume is composed)
4. Vertical slab roles (what each band represents)
5. Facade language (how the building communicates outward)
6. Environmental strategies (why form behaves the way it does)
7. Visual tone (how it should feel)
8. Material intent (what it implies, not what it simulates)

Only after these are resolved may geometry be emitted.

---

## 1. Urban-Scale Semantics

These define **city-level meaning** and influence silhouette, emphasis, and contrast.

```cpp
enum class BuildingUse {
    Office,
    Residential,
    Hotel,
    Civic,
    MixedUse,
    Industrial,
    Parking
};

enum class UrbanRole {
    Landmark,        // skyline anchor
    EdgeDefiner,     // shapes block edge
    Infill,          // background fabric
    Gateway,         // marks entrances / transitions
    Anchor,          // plaza / transit anchor
    Utility          // service / support building
};
```

Notes:
- `Landmark` implies stronger silhouette and crown emphasis
- `Infill` implies simpler massing and lower contrast
- These enums **do not map directly to geometry**

---

## 2. Site Relationship Semantics

These define **how the building relates to its parcel and streets**.

```cpp
enum class SitePlacement {
    CenteredObject,      // object-in-lot (modernist)
    EdgeAligned,         // street wall
    CornerEmphasis,      // rotated / cut corner
    Pavilion,            // free-standing object
    PodiumAndTower
};

enum class GroundInterface {
    Active,              // retail / lobby / transparency
    Permeable,           // pilotis / open ground
    Elevated,            // flood / infrastructure
    Sealed               // parking / service
};
```

These affect:
- footprint choice
- setback behavior
- ground-floor height
- omission vs extrusion of walls

---

## 3. Massing Grammar

Defines **how volume is composed**, not its exact dimensions.

```cpp
enum class MassingType {
    Slab,
    Tower,
    Courtyard,
    Stepped,
    Terraced,
    StackedVolumes,
    PodiumWithTower
};

enum class VerticalHierarchy {
    Uniform,
    PodiumDominant,
    TowerDominant,
    BaseMiddleCrown
};
```

Massing grammar determines:
- number of volumes
- step-back logic
- dominance of podium vs tower

---

## 4. Slab-Level Semantics (Critical)

Buildings are composed of **semantic slab bands**, not generic floors.

```cpp
enum class SlabRole {
    Infrastructure,   // flood base / utilities
    Podium,
    Public,           // lobby / civic
    Office,
    Residential,
    Terrace,
    Mechanical,
    Roof
};
```

Each slab role influences:
- thickness
- AO intensity
- color palette
- facade eligibility
- prop emission
- emissive behavior

---

## 5. Facade Language

Defines **expression**, not detail.

```cpp
enum class FacadeType {
    Solid,
    CurtainWall,
    Recessed,
    Screened,
    BriseSoleil
};

enum class FenestrationPattern {
    ContinuousBand,
    VerticalRhythm,
    HorizontalRhythm,
    Punched,
    None
};
```

Notes:
- Windows are never generated explicitly
- Facade intent drives omission or extrusion of surfaces

---

## 6. Environmental & Performance Semantics

These explain **why form behaves the way it does**.

```cpp
enum class EnvironmentalStrategy {
    None,
    Shaded,
    GreenTerrace,
    SolarRoof,
    PassiveCooling,
    FloodResilient
};
```

These typically map to:
- overhangs
- setbacks
- roof props
- raised bases

---

## 7. Visual Tone & Stylization

Controls **diorama-level feel**, not realism.

```cpp
enum class VisualTone {
    Cheerful,
    Neutral,
    Formal,
    Monumental,
    Playful
};

enum class ContrastLevel {
    Low,
    Medium,
    High
};
```

These affect:
- color saturation
- AO strength
- slab exaggeration
- emissive density

---

## 8. Material Intent (Not Materials)

This system does **not** select PBR materials.

```cpp
enum class MaterialIntent {
    Concrete,
    Glass,
    Stone,
    Metal,
    Greenery
};
```

Material intent maps to:
- vertex color palette
- AO defaults
- emissive eligibility

---

## 9. Unified Building Semantic Description

```cpp
struct BuildingSemantics {
    BuildingUse use;
    UrbanRole urbanRole;

    SitePlacement placement;
    GroundInterface ground;

    MassingType massing;
    VerticalHierarchy hierarchy;

    VisualTone tone;
    ContrastLevel contrast;

    std::vector<EnvironmentalStrategy> env;
};
```

This structure is **stable across engine lifetime** and must exist **before** geometry generation.

---

## 10. Slab Semantic Plan (Compiler Input)

Geometry generation operates on a **semantic slab plan**, not raw floor counts.

```cpp
struct SlabSemantic {
    SlabRole role;
    int startFloor;
    int floorCount;
};
```

Example:

```cpp
std::vector<SlabSemantic> plan = {
    {SlabRole::Infrastructure, 0, 1},
    {SlabRole::Public,         1, 2},
    {SlabRole::Office,         3, 6},
    {SlabRole::Terrace,        9, 1},
    {SlabRole::Mechanical,    10, 1},
    {SlabRole::Roof,          11, 1},
};
```

Only after this plan exists may geometry be emitted.

---

## 11. Compilation Model

Semantic compilation proceeds as:

```
BuildingSemantics
  ↓
Slab Semantic Plan
  ↓
Footprint + Placement Resolution
  ↓
Slab Geometry Rules
  ↓
Facade / Omission Rules
  ↓
Prop Emission
  ↓
LOD Derivation
```

At no point should mesh logic influence semantic decisions.

---

## 12. LOD Philosophy

LOD is **semantic collapse**, not mesh decimation.

Example:
- Far zoom → silhouette only
- Mid zoom → slab bands
- Near zoom → facade expression
- Close zoom → props + emissive

Dropping semantic layers is preferred over geometric simplification.

---

## 13. Debug & Validation Expectations

Agents and tools should support:
- coloring geometry by SlabRole
- visualizing footprint vs lot
- AO intensity visualization
- emissive mask visualization

If semantics are unclear, geometry should not be generated.

---

## Final Guidance for Agents

Prefer:
- explicit data
- boring, deterministic code
- semantic clarity
- stable visual language

Avoid:
- generic mesh booleans
- photoreal assumptions
- per-feature hacks
- hidden randomness

The goal is a **city compiler**, not an asset generator.

