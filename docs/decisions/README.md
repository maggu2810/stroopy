# Decision records

One file per decision: `NNNN-short-title.md` with *Status*, *Context*,
*Decision* and *Consequences* (optionally *Rationale*).

## Status

`proposed`, `accepted`, `postponed`, `superseded` or `rejected`, with the
date, plus links to later records that change it, e.g.
`accepted (2026-10-19); amended by 0012` or `superseded by 0014`.

## Records are append-only

A record documents why we decided something *at that time*. Once accepted:

- Never rewrite its substance. A changed decision gets a **new record**.
- Do update the old record's status line with links to the new record, so
  nobody acts on an outdated decision.
- Allowed in place: typos, broken links, wrong paths, and dated notes
  (`> **Note (YYYY-MM-DD):** …`) right below the status that point out what
  changed since.

The current state of all decisions is summarised in `AGENTS.md` ("Decided").
