# City Generator
[![ci](https://github.com/rfdickerson/city-generator/actions/workflows/ci.yml/badge.svg)](https://github.com/rfdickerson/city-generator/actions/workflows/ci.yml)

Engine-first procedural city builder with stylized, diorama-scale output. Geometry is generated
deterministically from semantic inputs (lots, buildings, streets, props), with a two-phase pipeline:
conceptual models first, mesh compilation second.

## Build

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

## Run

Single building:
```bash
./build/buildings config/sample_config.json
```

To select a preset (e.g., bungalow), set `"preset": "bungalow"` in `config/sample_config.json`.

City blocks:
```bash
./build/city_builder config/sample_city.json
```

Outputs are written to the working directory.
`outputName` in the config controls the glTF base filename.

## Styles

- `midcentury`
- `brutalist`
- `bungalow`

Set a style in `config/sample_config.json` or include it in `styles` for the city config.

## Tests

```bash
ctest --test-dir build
```

## Project Notes

- Determinism is required; the same inputs must produce identical output.
- Geometry prefers direct surface generation over CSG.
- Rendering expects vertex colors (RGB) and AO in alpha.
