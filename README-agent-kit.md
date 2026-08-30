# NES Survivor multi-agent kit v2

Cross-runtime layout:

```text
AGENTS.md                    # canonical project policy
GEMINI.md                    # Gemini adapter; imports AGENTS.md

.agents/
  roles/                     # provider-agnostic role definitions
    architecture-reviewer.md
    performance-reviewer.md
    implementation-worker.md
    final-reviewer.md
  skills/                    # shared Agent Skills
    caveman/
    nes-build-check/
    nes-budget/
    nes-runtime-check/
    implementation-note/

.codex/
  config.toml
  agents/                    # thin TOML adapters
    seu-camilo.toml
    relampago-marquinhos.toml
    ze-da-oficina.toml
    fiscal.toml

.gemini/
  agents/                    # thin Gemini CLI Markdown adapters
    seu-camilo.md
    relampago-marquinhos.md
    ze-da-oficina.md
    fiscal.md
```

## Design

One policy, one set of roles, one set of skills.

Provider-specific files only select runtime behavior and point back to shared roles.
This prevents Codex and Gemini from slowly developing different engineering rules.

See `MIGRATION.md` before replacing v1.
