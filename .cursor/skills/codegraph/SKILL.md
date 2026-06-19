---
name: codegraph
description: Code intelligence using codegraph - SQLite knowledge graph of code symbols, edges, and files. Use for exploring codebase, finding symbols, tracing call paths, understanding architecture. Must be used BEFORE writing or editing code.
paths: "*.{cpp,h,hpp,md,mdc,json}"
---

# Codegraph Usage Guide

## Overview

Codegraph is a SQLite knowledge graph that indexes every symbol, edge, and file in the workspace. Reads are sub-millisecond. The index lags writes by ~1 second through file watcher.

**Rule: Always consult codegraph BEFORE writing or editing code.**

## Tool Summary

| Tool | Purpose |
|------|---------|
| codegraph_explore | PRIMARY - Answers "how/where/what" questions with verbatim source |
| codegraph_search | Find symbol by name (returns kind + location + signature) |
| codegraph_node | Get full source of one specific symbol |
| codegraph_callers | Find what calls a function |
| codegraph_callees | Find what a function calls |
| codegraph_impact | Find what would break if you change something |
| codegraph_files | List files in a directory |
| codegraph_status | Check index status and size |

## Common Patterns

### 1. Understanding "How does X work?"
`
codegraph_explore({ query: "How does InputManager handle keyboard events?" })
`

### 2. Finding "Where is X defined?"
`
codegraph_search({ query: "CollisionSystem" })
`

### 3. Tracing "How does A reach B?"
`
codegraph_explore({ query: "InputManager PlayerController Physics" })
`

### 4. Impact Analysis (Before Refactoring)
`
codegraph_impact({ symbol: "Player" })
`

## Workflow

`
1. User asks about code - Use codegraph_explore FIRST
2. Found a symbol - Use codegraph_node for full source
3. Planning refactor - Use codegraph_impact
4. Only after understanding - Use Read/Grep for details
`

## Anti-Patterns

| Dont | Do |
|------|-------|
| Grep first for symbols | codegraph_search is faster and accurate |
| Read entire files blindly | codegraph_explore returns only relevant code |
| Chain search + node calls | ONE codegraph_explore returns grouped source |

## Limitations

- Index lags file writes by ~1 second
- Cross-file resolution is best-effort name matching
