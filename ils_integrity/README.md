ILS Signal Integrity — Extended Features

This update implements all requested improvements for the ILS Signal Integrity prototype. All features remain local and API-key-free.

What's new
----------
1) Frequency & Phase sliders: live controls to change simulated frequency (affects wavelength and arc spacing) and a global phase offset. Changes apply in real-time.
2) Phasor-based interference renderer: a higher-fidelity visual model that sums complex phasors from source(s) and reflected contributions to produce more physically-coherent interference ripples.
3) Plane sprite support & perspective transform: if the embedded plane PNG exists, the app uses it; otherwise falls back to vector plane. Perspective scaling and shadow improved.
4) Frame capture helper: a capture button that writes a sequence of PNG frames into ils_integrity/frames/ and a small helper shell script (capture_frames.sh) shows how to assemble them into an MP4/GIF using ffmpeg.
5) UI polishing: more controls and tooltips; README expanded with build/run and export instructions.

How to run
----------
1) Install dependencies (Ubuntu):
   sudo apt update
   sudo apt install -y build-essential pkg-config libgtk-3-dev ffmpeg
2) Build:
   cd computervisiondensity/ils_integrity
   make
3) Run:
   ./ils_integrity
4) To capture an animation (creates PNG frames in ./frames):
   Click "Capture" in-app or run: ./capture_frames.sh
   Combine frames into MP4: ffmpeg -framerate 24 -i frames/frame_%04d.png -c:v libx264 -pix_fmt yuv420p out.mp4
   Make GIF (optional): ffmpeg -i out.mp4 out.gif

Notes
-----
- The phasor model is simplified for visualization and runs on the CPU in real-time for modest arc counts. Increase/decrease N_ARCS for performance/quality tradeoffs.
- No external keys or services are required; the plane sprite (plane.png) is embedded as base64 and decoded on first run.

Files added/modified:
- ils_integrity/main.c (updated)
- ils_integrity/README.md (updated)
- ils_integrity/plane_base64.txt (embedded small PNG base64)
- ils_integrity/capture_frames.sh (helper script)

