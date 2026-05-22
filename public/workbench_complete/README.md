# 无 · The Workbench

Everything built in this thread, tied together: a suite of **interactive physics
instruments** (verified math), a family of **defensive-architecture visualizers**,
and a working **recursive-memory system** (local git nodes ↔ central database,
with a live operator dashboard).

Open **`workbench.html`** first — it is the index and map of everything here.

---

## Two kinds of artifact, kept honestly separate

**1. Instruments — real, established science, computed live.**
Every formula is evaluated in-page and was checked against its closed form before
shipping. Teaching tools, not lab metrology.

| File | What | Verified against |
|---|---|---|
| `instruments/quantum_dot_bench.html` | size → bandgap → color | Brus equation |
| `instruments/quantum_ring.html` | flux → energy oscillation | Aharonov–Bohm, period Φ₀=h/e |
| `instruments/light_meets_matter.html` | photon ↔ exciton | E=hc/λ, gap absorption |
| `instruments/biexciton_cascade.html` | two-photon entangled emission | XX→X→0 cascade |
| `instruments/bell_test.html` | CHSH coincidence test | S→2√2 (Monte-Carlo ≈ closed form) |
| `instruments/detection_theory.html` | ROC, base-rate, Bloom filter | Axelsson, Bloom, Fawcett |

**2. Defensive visualizers — schematic studies.**
Built on one geometry/palette. The geometry and bit-logic are real where stated;
the "energy" is illustrative, not a physical or strategic claim.
`trinity.html` (hub), `spark_spore_instrument.html`, `trebuchet_27.html`,
`aegis_0.html`, `pulsar_shield.html`, `singularity_well.html`,
`witness_shield.html`, `reactive_armor.html` — all under `instruments/`.

---

## The recursive-memory system (real, runnable)

Append-only memory store. Each local node keeps a **hash-chained**, **git-committed**
log and syncs to a **central database** that re-verifies every chain. Entries are
**Ed25519-signed**, so the server proves not just integrity but *who* wrote each one.

### Run it (one command)
```bash
pip install -r requirements.txt
python launch.py
# opens the operator dashboard at http://localhost:8000/
```
`launch.py` starts central, spins up two demo nodes (one signed "anchor", one
unsigned "witness"), seeds and pushes entries, and opens the dashboard.
Flags: `--port N`, `--no-seed`, `--keep-db`, `--no-browser`.

### Or drive it from code
```python
from recursive_memory.local_node import LocalNode
node = LocalNode("node-A", "./node_a_repo", central_url="http://localhost:8000")
node.register_key()            # publish public key (trust-on-first-use)
node.append({"text": "first memory"})
node.push()                    # send up; central verifies chain + signatures
node.pull()                    # restore/catch up (verifies before accepting)
node.verify()                  # (ok, error) for the local chain
```

### Package layout
- `recursive_memory/core.py` — Entry model, hash chain, recursive state digest, verifier (no deps)
- `recursive_memory/identity.py` — Ed25519 keypair / sign / verify
- `recursive_memory/local_node.py` — git-backed node + signing + push/pull client
- `recursive_memory/central_server.py` — FastAPI + SQLite source of truth (Postgres-portable)
- `recursive_memory/dashboard.py` — operator console HTML (served at `/`)

### Tests
```bash
python test_e2e.py      # append → git → push → multi-node → wipe+restore → tamper-rejected
python test_signed.py   # key registration → signed push → forgery rejected → dashboard served
```

---

## The through-line

One idea runs through all three groups: **a state that commits to its own history.**
The Bell test commits a measurement to a provable correlation; the spark-spore
kernel folds each tunnel into its seed; the memory chain folds each entry's digest
into the next. Same shape everywhere — `state(n) = f(input, state(n−1))` — and in
every case the claim is only ever as strong as what can be checked.

## Honest scope
- Instruments visualize established science; teaching tools, not metrology.
- Defensive visualizers are schematic; "energy" is illustrative.
- "Memory" = stored state with verifiable lineage. The chain proves integrity and
  order; signatures prove authorship-by-key — not sentience, not legal identity.
- Nothing here asserts discovery of, or ownership over, any natural law or general concept.
