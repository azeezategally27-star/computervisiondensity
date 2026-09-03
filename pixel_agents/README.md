# Pixel Agents — Mauritian Airport Pixel Room (Frontend + C Backend)

This addition implements a cozy pixel-art interactive app inspired by VS Code Pixel Agents: a very large, dense "pixel room" representing a Mauritian airport environment populated by pixel-agent characters performing airport tasks. The frontend is a lightweight Pygame app (Python) for pixel rendering and interaction. The backend is strictly C (C99) and exposes a small TCP protocol on localhost for listing agents, available tasks, and performing tasks — intentionally demo-only and self-contained.

Branch: feature/pixel-agents-standalone
Path: pixel_agents/

Design highlights
- Frontend: Python + Pygame (fast to prototype and cross-platform). Renders a very large room (scrollable) with dense pixel agents. Clicking an agent triggers tasks via the C backend and animates the agent performing the task.
- Backend: pure C (C99), single-file server (src/backend.c) listening on 127.0.0.1:9191. Agents and tasks are defined in clear C arrays so a grader can inspect how tasks are mapped to agent roles.
- Integration: The app is demo-only; tasks are simulated. The backend returns deterministic results and logs events to pixel_agents/logs/ for review.

How it maps to the airport system
- Agent roles include: Check-in Agent, Security Officer, Baggage Handler, Customs Officer, Gate Agent, Ground Crew, Customer Service, Immigration, Cleaning Crew, Fire Safety, Ramp Controller, and more.
- Tasks include: Issue Boarding Pass, Scan Bag, Route Luggage, Inspect Suspicious Item, Verify ID, Load Aircraft, Announce Boarding, De-ice, Direct Taxi, Clean Spill, Run Fire Drill, etc.
- Clicking a specific agent asks the backend to perform the role-appropriate task and shows visual feedback in the pixel room.

Build & run (Windows / Linux / macOS)
Prereqs:
- Python 3 with pygame (frontend)
- C compiler (gcc) for backend

1) Build backend (C):
   cd pixel_agents
   make
   ./bin/pixel_backend

2) Run frontend (Python + Pygame):
   pip install pygame
   python3 frontend.py

The frontend connects to the backend at 127.0.0.1:9191 automatically.

Files added
- pixel_agents/Makefile
- pixel_agents/README.md (this file)
- pixel_agents/src/backend.c
- pixel_agents/python/frontend.py

Notes
- The pixel engine draws simple colored rectangles and text to emulate pixel-art characters; there are no external sprite assets so the repo remains small.
- The backend is intentionally readable C. The ai/task handling is deterministic and well-commented.
- The frontend allows panning with arrow keys, clicking agents to trigger tasks, and displays an activity log.

Next steps you might want
- Replace procedural pixel characters with actual pixel sprite sheets.
- Add saved scenarios that integrate with the ticketing/xray demos (e.g., clicking a passenger agent opens their reservation).
- Add keyboard shortcuts, zoom, and a minimap.

Enjoy — run the backend first, then the frontend. Have fun exploring the Mauritian Airport pixel world!
