---
name: implementation-note
description: Create or update the required Brazilian-Portuguese NES Survivor implementation note for meaningful gameplay, architecture, performance, rendering, memory or hardware changes.
---

Write/update one focused file under `docs/implementation-notes/`.

Use the final implementation and actual validation as sources. Do not document abandoned code as current behavior.

Include only relevant sections:
- Problema;
- Solução adotada;
- Fluxo de execução;
- Trechos pequenos do código real;
- Restrições/trade-offs do NES;
- Desempenho;
- Impacto em RAM/Zero Page/PRG/CHR/OAM;
- O que observar no Mesen;
- Limitações/próximos passos.

Rules:
1. Brazilian Portuguese.
2. Explain why, not just what changed.
3. Keep snippets small and representative.
4. Separate measured values from estimates.
5. If performance was not measured, write `Impacto de desempenho: não medido.`
6. Never invent benchmark, cycle, memory or emulator results.
7. Update an existing note when it already owns the behavior; do not create competing notes.
8. Trivial typo/format/cosmetic changes do not need a note.
