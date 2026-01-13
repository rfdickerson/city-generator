# Mid-Century Modern Visual Language

## Purpose

This document defines the **visual and architectural language of Mid-Century Modern architecture** as a **procedural, semantic reference**.

It is intended to be used by:
- Automated agents (e.g. Codex)
- Procedural building generators
- Geometry compilers
- Rendering and LOD systems

This is **not** a photorealistic or historical reconstruction guide.
It is a **stylized, system-friendly interpretation** suitable for city builders, diorama-scale rendering, and custom engines.

---

## Core Architectural Ethos

> **Mid-Century Modern architecture expresses optimism, rational order, and openness through structure rather than ornament.**

Key ideas:
- Architecture is legible
- Structure is honest
- Form follows purpose
- Space and light are as important as mass

In procedural terms:
- Meaning precedes geometry
- Repetition is intentional
- Simplicity increases clarity

---

## High-Level Visual Characteristics

Mid-Century Modern buildings typically exhibit:

- Clear rectilinear geometry
- Orthogonal alignment
- Strong horizontal emphasis
- Repetition of structural elements
- Minimal surface detail
- Explicit separation of functional zones

They deliberately avoid:
- historical ornament
- decorative facades
- complex silhouettes
- surface noise

---

## Object-in-Space Principle

Mid-Century Modern buildings are usually **objects placed within space**, not shapes that tightly conform to parcel edges.

Implications:
- Buildings are centered or deliberately offset within lots
- Surrounding voids (plazas, landscaping) are intentional
- Negative space is part of the composition

Procedural guidance:
- Prefer rectangular or orthogonal footprints
- Place buildings inside lots, not flush to edges
- Shrink footprints relative to parcel bounds

---

## Massing Language

### Rectilinear Dominance

- Primary masses are rectangular slabs or towers
- Corners are sharp and unadorned
- Curvature is rare and secondary

Procedural guidance:
- Snap primary axes to 0° or 90°
- Avoid tracing irregular lot boundaries
- Favor simple rectangles over complex polygons

---

### Hierarchical Volume Composition

Mid-Century Modern buildings express hierarchy through stacked volumes:

- Base or podium
- Repetitive occupied mass
- Roof or crown element

Procedural guidance:
- Use step-backs to articulate hierarchy
- Change footprint size by vertical band
- Emphasize base and roof over middle floors

---

## Vertical Banding (Slab Logic)

Buildings read as **bands**, not individual floors.

Common bands:
- Ground / public interface
- Repetitive office or residential bands
- Terraces or setbacks
- Mechanical or roof bands

Procedural guidance:
- Group floors into 2–4 floor bands
- Maintain consistent slab thickness per band
- Avoid per-floor articulation

---

## Facade Language

### Curtain Wall Expression

- Large planar glass surfaces
- Minimal framing
- Continuous horizontal or vertical bands

Procedural interpretation:
- Treat glass as a single surface per band
- Do not model individual windows
- Uniform transparency within a band

---

### Structural Rhythm

- Structure is visible but abstracted
- Rhythm matters more than detail

Procedural interpretation:
- Express structure via repetition
- Use fins, slabs, or voids instead of beams

---

### Brise-Soleil (Sun Shading)

A defining mid-century feature.

Characteristics:
- Horizontal or vertical fins
- Regular spacing
- Strong shadow casting

Procedural guidance:
- Add thin extruded slabs every N floors
- Or vertical fins at fixed spacing
- Keep geometry thin and repetitive

---

## Ground Interface

Mid-Century Modern architecture often treats the ground level differently from upper floors.

Common strategies:
- Pilotis (columns lifting the building)
- Transparent or open lobbies
- Public plazas beneath the mass

Procedural guidance:
- Ground floors may be taller
- Walls may be omitted at ground level
- Columns are simple and widely spaced

---

## Roof Language

Roofs are explicit and legible.

Typical features:
- Flat roofs
- Strong edge caps
- Mechanical screening bands

Procedural guidance:
- Add a distinct roof slab
- Slightly exaggerate roof thickness
- Keep roof geometry simple

---

## Material Expression (Symbolic)

Mid-Century Modern uses **material implication**, not surface realism.

Common material intents:
- Concrete
- Glass
- Metal
- Stone (secondary)

Procedural guidance:
- Use flat or gently varied colors
- Avoid texture detail
- Encode materials via color and AO

---

## Color Language

Color palettes are typically:
- restrained
- optimistic
- low-frequency

Common choices:
- warm concrete grays
- soft whites
- muted blues and greens for glass
- occasional accent colors

Procedural guidance:
- Use color to distinguish bands
- Avoid high contrast within a single surface
- Preserve readability at distance

---

## Environmental Relationship

Mid-Century Modern buildings often respond to climate and context.

Common strategies:
- shading devices
- terraces and setbacks
- openness to light and air

Procedural guidance:
- Step-backs imply climate response
- Overhangs imply sun control
- Terraces imply human scale

---

## What Mid-Century Modern Is Not

To preserve visual clarity, avoid:

- Decorative window frames
- Dense mullion detail
- Brick or stone textures
- Irregular silhouettes
- Heavy ornamentation
- Expressive roof clutter

These elements push buildings toward postmodern or photoreal styles.

---

## Procedural Summary (For Agents)

When generating Mid-Century Modern buildings:

- Start with simple rectangular massing
- Place buildings as objects within lots
- Express hierarchy through bands, not detail
- Use slabs, fins, and voids as primary vocabulary
- Encode materials through color and AO
- Optimize for silhouette and rhythm

If an element does not read clearly from a distance, it should be removed.

---

## Guiding Question

Before emitting geometry, ask:

> **Does this element express structure, openness, or hierarchy?**

If not, it likely does not belong in Mid-Century Modern architecture.

---

## Final Note

This visual language is intended to be:
- stable
- legible
- procedural
- stylized
- scalable

Mid-Century Modern is not about detail.
It is about **clarity of form, optimism, and rational order**.

