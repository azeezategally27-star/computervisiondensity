ILS Signal Integrity — Native C prototype

This prototype implements a native desktop app (GTK3/Cairo) that visualizes an ILS-style guidance beam emitted from an antenna at the runway end. It provides an interactive canvas animation where a user can drag an airplane icon along the taxiway; when the aircraft intersects the emitted wave arcs, simple reflection vectors are computed and the wave pattern is visually distorted with interference ripples.

All simulation is local and requires no external APIs or keys.

Features included
- Animated concentric pulsing arcs (sine-wave style) emitted from an antenna at runway end.
- Draggable airplane icon across the taxiway; scale & shadow simulate simple 3D depth.
- Reflection/distortion: when aircraft intersects a wave arc, a reflected-origin ripple is generated and combined to produce visible interference ripples.
- Buttons to "Auto-correct Beam", "Apply Nulling" and "Reset" to demonstrate mitigation actions with animated effects.
- Descriptive README and quick build/run steps.

Notes
- This is a simulation/prototype designed for UI/animation review and teaching demos. Physics is simplified but visually convincing.
