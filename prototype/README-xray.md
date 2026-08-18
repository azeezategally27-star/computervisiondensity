# X‑Ray Security Metaverse — Prototype (C demo-only)

This extension is a separate prototype focused on a refined 3D animation of the X‑ray luggage security workflow for an airport, with simulated multi‑sensor fusion and a C-based AI stub (the "C-cheat"). It is intentionally demo-only and runs without external ML runtimes.

Branch: feature/xray-security-metaverse (this folder lives under prototype/)

Design highlights
- Language: C (C99), SDL2 + OpenGL for rendering (desktop native app). Code is readable C and contains a clear ai_stub where realistic detection behavior is synthesized.
- Theme: purple primary UI (hex #6A4AE2) with white panels and high-contrast overlays.
- Demo mode: deterministic or randomized simulated detections for instructor inspection. No external models required.
- Platform: Windows-first notes included; Linux/macOS also supported via provided Makefile adjustments.

How to build (Windows/MSYS2) — summarized
1. Install MSYS2: https://www.msys2.org/
2. In MSYS2 MinGW64 shell install deps:
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-mesa
3. Build:
   cd prototype
   make
4. Run:
   ./bin/xray_metaverse_demo --demo

Linux notes
- Install: libsdl2-dev libgl1-mesa-dev build-essential
- Then make as above.

Operation
- The app animates luggage on a conveyor, renders a synthetic X‑ray image when a bag passes the tunnel, runs the ai_stub which returns bounding boxes and classes (firearm, knife, liquid, battery, ied-like), and computes a fused threat score combining sensors (metal detector, weight).
- The operator UI (purple/white) shows detections, confidence, suggested actions, and contains buttons to "Hold Bag", "Open Manual Check", and "Release".
- Evidence snapshots are saved to prototype/evidence/ when a high threat is detected (un-encrypted as requested).

Files in this folder
- Makefile
- README-xray.md (this file)
- src/*.c / *.h (implementation)
- assets/sample_scenario.txt (bag compositions)

Ethics & safety
- Demo only: simulated data. For operational systems, follow legal/regulatory approvals.
- Logs/evidence are stored locally by default; secure and limit access for real deployments.

