Advanced Security Assistant — Mauritius Airport (Prototype)

This prototype is a native desktop application (C + GTK3 + SQLite) that simulates an advanced assistant for screening counterfeit items and suspicious behavior at Mauritius Airport. It is an educational/demo tool only — all detection and "AI" behavior is simulated and based on local, auditable rules and a small knowledge base. No external APIs or keys are used.

Important safety note
- This tool is intended for academic demonstration and UI/animation review. It does NOT provide operational guidance to commit wrongdoing, nor does it provide instructions to bypass security. The KB and heuristics are high-level and focus on detection, escalation, and safe procedures.

What’s included
- Animated canvas simulating a checkpoint area with passengers and luggage moving across frames.
- A local retrieval-based assistant (SQLite FTS5) that returns relevant guidance from local KB documents when an item is flagged.
- Interactive controls: Train KB, Scan Frame, Quarantine Item, Raise Alert; animated highlights for flagged items.
- Lightweight simulated "train" routine that ingests plain-text KB docs from advanced_security_assistant/kb/.

How to build and run (Ubuntu / Debian)
1) Install dependencies:
   sudo apt update
   sudo apt install -y build-essential pkg-config libgtk-3-dev libsqlite3-dev
2) Build:
   cd advanced_security_assistant
   make
3) Train KB (ingest sample guidance):
   ./security_assistant --train
4) Run the app:
   ./security_assistant

Where to look in the code
- main.c: UI, canvas animation, simulated detection heuristics, assistant integration.
- kb.c / kb.h: SQLite FTS5 knowledge base trainer and query functions.
- kb/*.txt: sample knowledge base documents (procedures & indicators) — intentionally non-actionable.

Limitations
- This is a simulation—no live airport feeds, no integration with databases or cameras. The intent is visual & architectural: smooth UI, animations, assistant retrieval, and realistic-feeling interactions that are safe for classroom evaluation.

If you want enhancements
- Add more KB docs (audit logs, SOPs) to improve assistant responses.
- Hook up to a local camera feed or recorded video (offline) for frame-by-frame analysis (still simulated — no external services).
- Export alerts and audit logs to CSV/PDF for reporting.
