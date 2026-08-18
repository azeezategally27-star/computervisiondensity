# Advanced Flight Assistant Panel — Extended Features

This extension adds multiple simulated, highly‑animated operational panels to the Flight Assistant prototype. All features are local and simulated — no external API keys or services required. The goal is to present a visually-rich, smooth UI demonstrating the requested advanced features for evaluation.

What was added
---------------
- Gate Map: animated gate map with taxiing aircraft icons (simulated).
- Baggage Panel: conveyor visualization with animated baggage items and status per flight.
- Assistant: local rule-based AI assistant with "typing" animation and action suggestions.
- Scheduler: animated Gantt-like resource scheduler visualization.
- UI Buttons to open each panel and connect to simulator data.

All windows are native GTK3/C and run locally. The simulator drives animations and state changes.

How to build
------------
Dependencies (Ubuntu):
  sudo apt update
  sudo apt install -y build-essential pkg-config libgtk-3-dev libsqlite3-dev

Build and run inside advanced_assistant/:
  make
  ./flight_assistant

No API keys required — everything is simulated and self-contained.
