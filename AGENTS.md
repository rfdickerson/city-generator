# AGENTS.md

## Purpose

This repository is an **engine-first city builder project** focused on:

- Procedural generation of cities (lots, buildings, streets, props)
- Stylized, diorama-scale visuals (SimCity 2013 / Up / Zootopia)
- Deterministic, data-driven geometry
- A small, understandable codebase with minimal dependencies

This is **not** a general-purpose game engine.
Every system exists to serve *city-building semantics*.

Agents contributing here should optimize for:
- clarity
- stability
- readability at distance
- system composability

---

## Core Design Philosophy

### 1. Geometry Is Generated, Not Modeled

- Buildings, roads, props, and clutter are generated procedurally
- No hand-authored meshes except for tiny instanced props
- Avoid general-purpose CSG; prefer direct surface generation
- If geometry can be *omitted* instead of subtracted, do that

> Generate the final surface directly.

---

### 2. City Semantics Come First

The engine understands:
- lots
- blocks
- buildings
- streets
- zones
- props

It does **not** treat everything as generic meshes.

Rendering, LOD, and shading are **semantic-aware**.

---

### 3. Diorama-Scale Visual Language

Visual goals:
- clean silhouettes
- chunky proportions
- strong color blocking
- minimal texture noise
- stable shading at all zoom levels

Avoid:
- photorealism
- PBR pipelines
- SSAO-heavy rendering
- fine surface detail

This city should look like a **physical architectural model**.

---

### 4. Determinism Is Non-Negotiable

Given the same:
- seed
- input data
- parameters

The output **must be identical**.

This applies to:
- geometry
- colors
- AO
- emissive patterns
- prop placement

No hidden randomness.

---

## Geometry Rules

### Allowed Geometry Operations

- 2D polygon inset / outset
- Oriented bounding rectangles
- Vertical extrusion
- Slab stacking
- Step-backs via footprint reduction
- Box-based props
- Surface omission instead of boolean subtraction

### Avoid

- Arbitrary mesh boolean CSG
- Concave polygon difference
- Triangle-level mesh surgery
- Runtime mesh mutation

If a feature requires full CSG, reconsider the design.

---

## Buildings

Buildings are **objects placed inside lots**, not shapes that trace lot boundaries.

Typical structure:
- infrastructure / base
- podium
- tower
- terraces
- roof systems

Buildings should prefer:
- rectangular or orthogonal footprints
- clear mass hierarchy
- negative space around them

---

## Shading & Materials

### Lighting Model

- Lambertian diffuse only
- Strong ambient contribution
- Optional hemispheric ambient
- No specular highlights

### Vertex Data

Each vertex encodes **meaning**:

- `color.rgb` → base material color
- `color.a`   → static ambient occlusion
- `uv`        → dirt, decals, subtle overlays

AO is **baked procedurally**, not computed in screen space.

---

## Night Mode & Emissive

- Emissive windows are procedural
- Stored as vertex data (or secondary color channel)
- Night/day is a smooth blend, not a toggle

Avoid:
- per-window geometry
- emissive textures
- dynamic lights per building

---

## Roads & Streets

- Road markings use vertex colors by default
- UVs + decals only for symbols (arrows, crosswalks)
- No asphalt textures
- Streets are graphic design, not material simulation

---

## Props & Clutter

Props are **instanced hints**, not detailed assets.

Examples:
- cars → colored boxes
- light poles → simple cylinders + emissive head
- benches, trash cans → box primitives
- power lines → sparse line segments

Placement is **rule-based**, not random.

---

## LOD Strategy

LOD is **semantic**, not just distance-based.

Example:
- city zoom → boxes only
- neighborhood zoom → slabs + massing
- street zoom → props + emissive
- close zoom → optional detail

Dropping entire layers is preferred over mesh decimation.

---

## Performance Expectations

- Massive instancing is expected
- Renderer should batch by semantic category
- Geometry is mostly static and can be baked
- Runtime cost should scale with *visible meaning*, not triangle count

---

## What This Repo Is NOT

- Not a character engine
- Not a physics sandbox
- Not a cinematic renderer
- Not a photoreal simulation

If a contribution pushes the project in that direction, it should be rejected or heavily reconsidered.

---

## Guiding Question for All Changes

Before adding code, ask:

> Does this improve city readability, clarity, or expressiveness at scale?

If the answer is no, it probably doesn’t belong here.

---

## Style Inspiration

This project draws inspiration from:
- SimCity (2013)
- architectural dioramas
- mid-century modern & brutalist massing
- animated cities (Up, Zootopia)

Not from:
- FPS engines
- film VFX pipelines
- CAD software
- real-world material scanning

---

## Final Note for Agents

Prefer:
- simple math
- explicit data
- clear intent
- boring code that works

Over:
- clever abstractions
- generic solutions
- premature optimization
- black-box libraries

The goal is a **city compiler**, not a tech demo.

# Repository Guidelines

## Project Structure & Module Organization
- `CMakeLists.txt` defines a single C++20 executable target named `buildings`.
- `.clang-format` provides the formatting rules; `.idea/` and `cmake-build-debug/` are IDE/build outputs and should not be edited by hand.
- Output: running the program writes `simcity_midcentury_office.obj` to the working directory.

## Build, Test, and Development Commands
- Configure: `cmake -S . -B build` generates build files in `build/`.
- Build: `cmake --build build` compiles the `buildings` executable.
- Run: `./build/buildings config/sample_config.json` writes the OBJ file and prints a status line.
- CLion users can build/run via the IDE using the existing `cmake-build-debug/` directory.

## Coding Style & Naming Conventions
- Indentation: 4 spaces, no tabs. Keep short, single-purpose functions where practical.
- Naming: `UpperCamelCase` for types (`Vec3`, `Mesh`), `UpperCamelCase` for free functions (`BuildSlab`), and `lowerCamelCase` for locals (`uvScale`).
- Formatting: run `clang-format -i main.cpp` to align with `.clang-format`.

## Testing Guidelines
- No automated tests are present. If adding tests, prefer a lightweight approach (e.g., a small geometry sanity check) and wire it via CMake/CTest.
- Suggested naming: `test_<feature>.cpp` (e.g., `test_slab.cpp`).

## Commit & Pull Request Guidelines
- No Git history exists in this working copy, so there are no established conventions.
- If you initialize Git, use short, imperative commit subjects (e.g., "Add curtain wall UV scaling") and keep PRs focused.
- Include: a brief description, steps to run, and an example output filename when behavior changes.

## Configuration Tips
- The executable writes files to the current working directory; run from a writable location.
- If you adjust geometry parameters, note the change in the output OBJ name or document it in the PR.
