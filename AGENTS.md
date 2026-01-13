# Repository Guidelines

## Project Structure & Module Organization
- `main.cpp` contains all geometry generation code (math helpers, slab/canopy builders, OBJ writer, and `main`).
- `CMakeLists.txt` defines a single C++20 executable target named `buildings`.
- `.clang-format` provides the formatting rules; `.idea/` and `cmake-build-debug/` are IDE/build outputs and should not be edited by hand.
- Output: running the program writes `simcity_midcentury_office.obj` to the working directory.

## Build, Test, and Development Commands
- Configure: `cmake -S . -B build` generates build files in `build/`.
- Build: `cmake --build build` compiles the `buildings` executable.
- Run: `./build/buildings` writes the OBJ file and prints a status line.
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
