---
name: project-logs
description: Use when finishing a task, making a commit, or taking a design decision in the rp569 repo — keeps .claude/todo.md, done.md, commits.md and decisions.md up to date.
---

# Project logs (rp569)

## When a task is finished
1. Remove/check it in `.claude/todo.md`.
2. Add `- YYYY-MM-DD — <what>` at the top of `.claude/done.md`.

## When committing
1. Stage the work **and** the log updates together.
2. Add an entry at the top of `.claude/commits.md`:
   ```
   ## <short-hash> — YYYY-MM-DD — <subject>
   What: files/areas changed.
   Why: intent, link to todo/decision id.
   ```
   The hash is not known before committing: write `## pending — ...`, commit, then replace
   `pending` with the real hash at the start of the next commit (do not amend).

## When making a design choice
Add a `D-NNN` entry to `.claude/decisions.md` with the datasheet section used as source.
