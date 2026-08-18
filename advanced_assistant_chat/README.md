Advanced Assistant Chat — Native C GTK3 App (Offline)

This component is a native desktop chatbot app written in C (GTK3 + SQLite) that simulates an advanced airport assistant for Mauritius airport operations. It runs fully offline (no API keys) and uses a local retrieval-based "training" pipeline: text documents are ingested into a local SQLite FTS5 knowledge base and used at query time to produce informed, context-aware replies. The UI includes smooth animations: a breathing avatar, typing indicator, and animated message fade-in for realistic interaction.

Features
- Offline retrieval-augmented assistant (SQLite FTS5) — ingest local SOPs, manuals, and examples.
- Rule-based response composer that combines retrieved passages with safety templates.
- Animated UI: typing indicator, message fade-in, avatar breathing.
- Trainer tool: load all .txt files in advanced_assistant_chat/kb/ into the local DB (no external models required).
- All local; no API keys required. Good for demo & grading.

Build & run
1) Install deps (Ubuntu):
   sudo apt update
   sudo apt install -y build-essential pkg-config libgtk-3-dev libsqlite3-dev
2) Build:
   cd advanced_assistant_chat
   make
3) Train the KB (ingest sample docs):
   ./assistant_chat --train
4) Run the app:
   ./assistant_chat

How the "AI" works (brief)
- The "training" step inserts each KB text file into an SQLite FTS5 table.
- At query time the app performs an FTS5 match against the user message and fetches the top N matching passages.
- The response composer selects and stitches helpful snippets, applies small heuristics (clarify if user asked for scheduling, provide suggested steps), and returns the composed text in animated chunks.
- This hybrid approach is deterministic, auditable, and easy to inspect — ideal for a classroom demo where the teacher examines code + UI.

Files
- main.c: GTK3 UI, animations, and program entrypoint.
- kb.c / kb.h: SQLite FTS5 knowledge base and search utilities.
- Makefile: build rules.
- resources.css: theme styles for the chat UI.
- kb/*.txt: sample knowledge base documents (SOPs, flight operations examples) used for training.

Notes & limitations
- This is not a neural LLM — training is indexing documents for retrieval and applying templates. It is intentionally offline, explainable, and safe for academic use.
- The UI animations are implemented with GTK timeouts and opacity interpolation for smooth transitions.

If you want, I can now:
- Expand the KB with more domain docs you provide.
- Add an offline small NN inference (requires bundled model, not possible without large files).
- Convert the reply composer to a lightweight Markov or seq2seq generator (still pure C) for more varied replies.
