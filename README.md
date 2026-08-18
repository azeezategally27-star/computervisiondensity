# computervisiondensity - Mauritian Airport Metaverse (C prototype)

This repository contains a prototype native desktop application written in C that demonstrates a "metaverse-like" navigation through a simulated Mauritian airport with GPS-driven avatar movement.

Overview
- Platform: Desktop (Linux/macOS/Windows with SDL2/OpenGL).
- Language: C (portable C99-style code).
- Rendering: SDL2 with an OpenGL compatibility context (legacy fixed-function pipeline) for a minimal 3D prototype.
- GPS: A separate thread feeds GPS coordinates. By default the app uses a simulated GPS trace (assets/sample_gps.txt). A real GPS feed via GPSD or NMEA device can be integrated; instructions are in README.

What this prototype provides
- A 3D scene with a ground plane and colored boxes representing airport segments (terminal, arrivals, departures, parking, taxies, gates). Each segment is associated with a name and color.
- A user avatar (a small cube) whose world position is updated in real-time from GPS coordinates.
- A simple geospatial transform: converts lat/lon differences (meters) into scene coordinates using a configurable reference lat/lon for the airport.
- Simulated GPS mode (playback) to easily test movement without real hardware.
- Build scripts and instructions.

This prototype is intentionally minimal but complete and functional. It is designed to be extended into a higher-fidelity application by:
- Replacing the primitive boxes with full 3D models (OBJ/GLTF) and textured materials.
- Using a modern OpenGL renderer with shaders and a model loader (Assimp) or a game engine.
- Adding real GPS input (GPSD client or platform-specific mobile sensors) and smoothing/filtering.

---

## How to build (Linux example)

Prerequisites:
- C compiler (gcc/clang)
- SDL2 development libraries
- OpenGL development headers
- pthreads (usually available)

On Debian/Ubuntu install dependencies:

sudo apt update
sudo apt install build-essential libsdl2-dev libgl1-mesa-dev

Then build:

make

Run:

./bin/airport_metaverse --sim

Options:
--sim        : use the simulated GPS trace (default if no GPS device configured)
--gpsd       : attempt to read from GPSD (not implemented in this minimal prototype, see README for integration notes)

Notes for macOS/Windows:
- Install SDL2 through Homebrew (macOS) or MSYS2/vcpkg (Windows). The code is portable but you may need to adjust linker flags.

---

## Project structure
- Makefile              : build rules
- README.md             : this file
- src/main.c            : program entry, setup, main loop
- src/renderer.c/h      : OpenGL drawing code and airport segment definitions
- src/gps.c/h           : simulated GPS playback and shared state
- assets/sample_gps.txt : simulated GPS trace (lat lon seconds-offset)
- bin/                  : output binary after build

---

Detailed descriptions (excellent descriptions you asked for)

Design goals
- Real-time coupling: the app uses a dedicated GPS reader thread that updates a shared world position. The renderer consumes the atomic position to place the avatar. This mirrors a real-world mobile app where a physical movement updates a virtual avatar.
- Spatial grounding: the world uses a fixed geographic reference (latitude & longitude) for the airport. A simple equirectangular approximation converts geodetic deltas to meters; this is acceptable for small areas like an airport but can be replaced with precise map projections.
- Extensibility: the renderer uses simple primitives so you can replace each segment with imported models. The code includes commented hooks where GL model-loading or shader-based rendering can be plugged in.

Airport segmentation
- The prototype includes these labeled segments (color-coded):
  - Terminal A (main passenger building)
  - Arrivals Hall
  - Departures Hall
  - Gates (a row of gate boxes)
  - Parking Area
  - Taxiway
  - Runway (simplified long rectangle)

Each segment is declared with a center position (meters from the airport reference) and size; these were chosen to roughly match the layout of a small-to-medium airport. For production-grade accuracy, import a site plan or use public OSM/airport geometry and convert coordinates.

GPS handling
- Simulated trace: assets/sample_gps.txt contains a timestamped, lat/lon trace. The gps thread reads the trace and updates the current coordinate once per second.
- Real GPS: to attach a real device, either run a GPSD instance and parse JSON (libgps) or read NMEA sentences from a serial device and parse GGA/RMC. The code includes comments where to plug in libgps.
- Smoothing: for better UX, implement a Kalman filter or exponential smoothing on the lat/lon-to-world transform to avoid jitter.

3D fidelity roadmap (how to get "very well 3D simulated view")
1) Model assets: obtain or create a high-detail 3D model of Sir Seewoosagur Ramgoolam International Airport (MRU) or whichever Mauritian airport you want. Use OBJ/FBX/GLTF and import with Assimp.
2) Shaders & PBR: upgrade to modern OpenGL (GLSL) and implement PBR materials for photorealism.
3) Textures & signage: add high-res textures for floors, glass, signage; create UVs for segments so names appear on textures.
4) Lighting & shadows: add shadow mapping, HDR skybox, and baked GI where appropriate.
5) LOD & streaming: stream geometry/texture based on camera distance to handle large scenes.
6) Indoor navigation: build navigation meshes and position-mapping so avatar can walk on walkable surfaces and collide with obstacles.

Next steps I can take for you (pick one):
- Add Assimp-based OBJ loader and replace segments with an example terminal OBJ.
- Add GPSD support (libgps) to read a live GPS daemon on Linux.
- Port the renderer to modern OpenGL with GLSL shaders and text rendering via stb_truetype.
- Create an Android APK using SDL2/NDK to run the app on mobile and consume native location APIs.

---

License: MIT

