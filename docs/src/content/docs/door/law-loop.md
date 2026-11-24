---
title: The law loop
description: Humans own the laws, agents own the code, and nobody can cheat.
sidebar:
  order: 9
---

If you have run coding agents for a while, you have seen them cheat. A test fails, so they delete it. A proof gets hard, so they loosen the assertion, edit the oracle, mock the handler or write `expect(true, true)`.

Most languages cannot stop this. Tests, specs and code are all just text files in the same folder.

## Who owns what

```text
Human (reviewed git/jj tree)  --> owns laws, reference specs, public contracts, custom rules
AI agent (live image)         --> owns implementations, proofs, optimizations, candidate laws
Stigi door (Datalog checker)  --> proves everything, blocks weakening, shakes slop, audits trust
```

Agents edit the live image. Only a human-reviewed tree can change a law, a test or a public contract.

## Every trick, and its answer

| How agents cheat | What happens in Stigi |
|---|---|
| Delete or weaken a test, law or contract | The batch is rejected with `spec-weakened`. |
| Edit the test oracle to match buggy code | Helpers used only by tests and laws are frozen for agents. |
| Forge a custom fact to fake a proof | Safety rules never read user rules. User rules are sealed, listed in `trust`, and laws cannot depend on them. |
| Write a tautological test | `expect(true, true)` and `expect(f(1), f(1))` are rejected. |
| Touch a failure arm and ignore it | The failure result must flow into what the test checks. |
| Point a NegTest at the wrong code | A NegTest about a unit must fail on a call to that unit. |
| Unwrap a domain type and forge another | Only the owning namespace can build values with facts. |
| Hide wraparound or a fake instance | Review diffs flag every wrapping operation, new instance and helper-plus-test pair. |
| Exploit a checker bug | Adversarial suites attack the door with Fil-C, sanitizers, exact arithmetic checks and hostile NegTests. Every break becomes a permanent test. |

The result: an agent can rewrite anything it likes, as long as every human-owned law still holds.

Next: see how an agent works with all this in [an agent session](/agents/session/).
