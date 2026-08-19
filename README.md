Airport Multi-Agent Simulator (pure C)
=====================================

Overview
--------
This is a pure-C multi-agent simulation of an airport system:
- CoordinatorAgent: central decision-making "AI" agent
- GateAgent: manages gates, assigns flights
- BaggageAgent: routes baggage autonomously
- SecurityAgent: scales lanes based on passenger load
- MonitorAgent: displays system status

Build
-----
Requirements: gcc, pthreads, POSIX environment.

$ make

Run
---
$ ./airport_sim data/flights.csv

The simulator reads flights from the CSV and simulates arrivals, check-ins,
baggage drop, gate allocation and boarding. Agents communicate via an
in-memory message bus (pthread-based).

Files
-----
- src/*.c, src/*.h : source
- data/flights.csv : sample flights

Design notes
------------
- Pure C, event-driven, multi-threaded. Agents are threads with message queues.
- Coordinator uses simple heuristics to assign gates and trigger other agents.
- Intended as a foundation for adding more sophisticated decision logic,
  constraint solving, or external model integrations (the architecture supports
  adding "tools" or plugin modules).
