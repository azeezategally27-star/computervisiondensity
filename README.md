This repository now contains two C applications:

1) airport_sim - the original multi-agent simulator (message-bus based)
2) biometric_app - an interactive, ncurses-based biometric UI and agent graph

Build
-----
Requires: gcc, pthreads, ncurses

$ make

Run
---
$ ./airport_sim data/flights.csv
$ ./biometric_app

biometric_app
-------------
- Provides a terminal-based visual graph of agents (Coordinator, BiometricAgent,
  DoorAgent, Monitor).
- Interactive buttons (keyboard shortcuts) at the bottom:
  - E : Enroll a passenger biometric (prompts for passenger ID)
  - V : Verify a passenger (prompts for passenger ID)
  - L : Clear logs shown in the UI
  - Q : Quit

Design notes
------------
- Implemented in pure C using pthreads and ncurses for a visual UI (no web).
- Uses the same msgbus message passing primitives as airport_sim.
- Biometric agent stores simple templates in memory and responds to enroll/verify.
- Monitor agent is implemented inside the UI app to collect logs and display them.

Next steps you can request
-------------------------
- Persist biometric templates using SQLite
- Add more detailed agent diagrams or export as ASCII/PNG
- Add cryptographic signing for templates
- Integrate simulated camera/fingerprint sensors (feed files)
