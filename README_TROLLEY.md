# ComputerVisionDensity - Mauritius Airport Trolley 3D App

This commit adds a cross-platform C prototype app that visualizes and simulates trolley operations at a Mauritian airport. It's written in plain C using raylib (https://www.raylib.com/) for simple cross-platform 2D/3D rendering and input handling.

Features implemented in this prototype:
- 3D scene with ground, taxiway, and simple terminal geometry.
- Multiple trolley objects that move along scheduled routes with smooth interpolation.
- Mouse-based drag & drop: click and drag a trolley to reposition it on the taxiway.
- Collision detection and simple avoidance: trolleys slow down when near each other.
- UI controls: Add trolley, Remove trolley, Pause/Resume, Reset, Optimize Paths (heuristic), Toggle 3D Camera.
- Color-coded trolley states (Idle, Moving, Dragged, Needs Service).
- Smooth animations: easing and sinusoidal bobbing for realism.
- Path visualization (routes) and heatmap-like density visualization.

Limitations & notes:
- This is a prototype. It requires raylib to build and run. No external API keys or online services are used.
- "Optimize Paths" runs a local heuristic to re-route trolleys; it's not AI-trained.
- The app is fully local and intended as a starting point; it does not include trained AI models or networked live data from the actual Mauritius airport.

Build & run (Linux/macOS with raylib installed):

1. Install raylib (follow instructions on https://www.raylib.com/). On many Linux distros you can build from source or install via package manager.
2. From the repo root:
   make -C src
   ./src/trolley_app

If you want a Windows build, use MinGW or Visual Studio and link against raylib.

Files added:
- src/main.c       (full prototype application)
- src/Makefile     (simple makefile)
- README_TROLLEY.md (this file)

Future work (possible next steps):
- Add more detailed 3D models and textures (GLTF/OBJ loader).
- Integrate an offline assistant using a small local model or rule-based engine.
- Record/replay trolley sessions and export telemetry.

