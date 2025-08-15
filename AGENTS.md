# AGENTS.md - openFrameworks Addon: ofxParticleSet

## Build Commands
- Build: `make` (from example directories)
- Clean: `make clean`
- Debug build: `make Debug`
- Release build: `make Release`
- No specific test framework - test via example projects

## Code Style & Conventions
- Language: C++ (openFrameworks style)
- Headers: `#pragma once` guard
- Includes: Standard includes first, then OF, then local headers
- Naming: camelCase for variables/functions, PascalCase for classes
- Member initialization: Use braced initialization lists in constructors
- Constants: UPPERCASE with underscores (e.g., `STRATEGY_POINTS`)
- Indentation: 2 spaces, no tabs
- Braces: Same line for functions/classes, new line for control structures

## Type Usage
- Vectors: Use `glm::vec2` for 2D positions/velocities
- Colors: Use `ofFloatColor` for particle colors
- Smart pointers: `shared_ptr` for spatial index management
- STL containers: `std::vector` for collections

## Error Handling
- Use const correctness (`const` member functions where appropriate)
- Thread safety via `lock()`/`unlock()` for mesh operations
- Lifetime management via `isAlive()` checks

## Dependencies
- Requires: ofxSpatialHash addon
- Core OF version: v0.12+