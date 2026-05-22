#!/usr/bin/env python3
"""
launch.py  —  one command to bring the whole recursive-memory system up.

What it does, in order:
  1. starts the central server (FastAPI) on a chosen port
  2. waits until it answers
  3. spins up two local git-backed nodes ("anchor" = signed, "witness" = unsigned)
  4. seeds each with a few memory entries and pushes them up
  5. opens the operator dashboard in your browser

Run:
    python launch.py                # default port 8000, fresh demo DB
    python launch.py --port 8123    # custom port
    python launch.py --no-seed      # start empty (no demo nodes)
    python launch.py --keep-db      # don't wipe the DB on start

Everything is local. Nothing leaves your machine.
"""
from __future__ import annotations
import argparse, os, sys, time, threading, tempfile, webbrowser


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=8000)
    ap.add_argument("--no-seed", action="store_true")
    ap.add_argument("--keep-db", action="store_true")
    ap.add_argument("--no-browser", action="store_true")
    args = ap.parse_args()

    db_path = os.path.abspath("central_memory.db")
    if not args.keep_db and os.path.exists(db_path):
        os.remove(db_path)
        print(f"[launch] fresh DB ({db_path})")
    os.environ["RMEM_DB"] = db_path

    try:
        import uvicorn, requests
        from recursive_memory import central_server
        from recursive_memory.local_node import LocalNode
    except ImportError as e:
        print(f"[launch] missing dependency: {e}\n  run:  pip install -r requirements.txt")
        sys.exit(1)

    central_server.init_db()
    url = f"http://127.0.0.1:{args.port}"

    # ---- start server in a background thread ----
    cfg = uvicorn.Config(central_server.app, host="127.0.0.1", port=args.port, log_level="warning")
    server = uvicorn.Server(cfg)
    threading.Thread(target=server.run, daemon=True).start()
    print(f"[launch] central server starting on {url} …")

    for _ in range(80):
        try:
            if requests.get(url + "/stats", timeout=1).ok:
                break
        except Exception:
            time.sleep(0.1)
    else:
        print("[launch] server did not come up"); sys.exit(1)
    print("[launch] central server is live")

    # ---- seed demo nodes ----
    if not args.no_seed:
        workdir = tempfile.mkdtemp(prefix="rmem_nodes_")
        print(f"[launch] node repos in {workdir}")

        anchor = LocalNode("anchor", os.path.join(workdir, "anchor"), central_url=url)
        anchor.register_key()
        for phase in ["wake", "observe", "reflect", "cohere", "return"]:
            anchor.append({"phase": phase})
        print("  anchor (signed):", anchor.push())

        witness = LocalNode("witness", os.path.join(workdir, "witness"), central_url=url)
        for ev in ["boundary check", "gate traversal", "anchor compare"]:
            witness.append({"event": ev})
        print("  witness (unsigned):", witness.push())

        print("[launch] seeded:", requests.get(url + "/stats").json())

    # ---- open dashboard ----
    if not args.no_browser:
        try:
            webbrowser.open(url + "/")
        except Exception:
            pass
    print(f"\n  ► Operator dashboard:  {url}/")
    print( "  ► Press Ctrl+C to stop.\n")

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\n[launch] shutting down")
        server.should_exit = True


if __name__ == "__main__":
    main()
