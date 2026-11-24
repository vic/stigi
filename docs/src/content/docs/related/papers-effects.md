---
title: "Papers: effects and concurrency"
description: Effect handlers, capabilities, race freedom, determinism and replay.
sidebar:
  order: 13
---

Stigi tracks effects as capabilities and runs them through handlers, and its parallel code gives the same answer as its sequential code. These papers show how effects, capabilities and race freedom fit together.

## Effect handlers and rows

- [Lucassen and Gifford (1988)](https://scholar.google.com/scholar?q=Lucassen+Gifford+1988+Polymorphic+effect+systems) — Lucassen and Gifford, POPL 1988.
- [Launchbury and Peyton Jones (1994)](https://scholar.google.com/scholar?q=Launchbury+Peyton+Jones+1994+Lazy+functional+state+threads) — Launchbury and Peyton Jones, PLDI 1994.
- [Handlers of Algebraic Effects](https://scholar.google.com/scholar?q=Handlers+of+Algebraic+Effects+Plotkin+Pretnar) — Plotkin and Pretnar, ESOP 2009.
- [Liberating Effects with Rows and Handlers](https://scholar.google.com/scholar?q=Liberating+Effects+with+Rows+and+Handlers) — Hillerstrom and Lindley, 2016.
- [Type Directed Compilation of Row-Typed Algebraic Effects](https://scholar.google.com/scholar?q=Type+Directed+Compilation+of+Row-Typed+Algebraic+Effects) — Leijen, POPL 2017.
- [Generalized Evidence Passing for Effect Handlers](https://scholar.google.com/scholar?q=Generalized+Evidence+Passing+for+Effect+Handlers) — Xie and Leijen, ICFP 2021.

## Capabilities and capture

- [Devriese, Birkedal and Piessens (2016)](https://scholar.google.com/scholar?q=Devriese+Birkedal+Piessens+2016+Reasoning+about+object+capabilities+with+logical+relations+and+effect+parametricity) — Devriese, Birkedal and Piessens, 2016.
- [Effects as Capabilities](https://scholar.google.com/scholar?q=Effects+as+Capabilities+Effect+Handlers+and+Lightweight+Effect+Polymorphism) — Brachthauser, Schuster and Ostermann, OOPSLA 2020.
- [System C](https://scholar.google.com/scholar?q=Brachthauser+Schuster+Lee+Boruch-Gruszecki+System+C+Effect+safety+OOPSLA+2022) — Brachthauser, Schuster, Lee and Boruch-Gruszecki, PACMPL 2022.
- [Capture checking for Scala 3](https://scholar.google.com/scholar?q=Boruch-Gruszecki+Odersky+Brachthauser+Lee+Lhotak+Capturing+Types+TOPLAS+2023) — Boruch-Gruszecki, Odersky, Brachthauser, Lee and Lhotak, TOPLAS 2023.
- [Tracking Capabilities for Separation and Freshness](https://scholar.google.com/scholar?q=Tracking+Capabilities+for+Separation+and+Freshness) — Xu, Bracevac, Pham, Zhao and Odersky, Scala Workshop 2025.
- [System Capless](https://scholar.google.com/scholar?q=Bracevac+Xu+Odersky+System+Capless) — Bracevac, Xu, Odersky et al., OOPSLA.

## Race freedom and structured concurrency

- [Bernstein (1966)](https://scholar.google.com/scholar?q=Bernstein+1966+Analysis+of+programs+for+parallel+processing) — Bernstein, 1966.
- [CRDTs](https://scholar.google.com/scholar?q=Shapiro+2011+Conflict-free+Replicated+Data+Types) — Shapiro et al., 2011.
- [Betts, Chong, Donaldson, Qadeer and Thomson (2012)](https://scholar.google.com/scholar?q=Betts+Chong+Donaldson+Qadeer+Thomson+2012+GPUVerify+verifier+for+GPU+kernels) — Betts, Chong, Donaldson, Qadeer and Thomson, OOPSLA 2012.
- [LVars](https://scholar.google.com/scholar?q=Kuper+Newton+LVars+lattice-based+data+structures+for+deterministic+parallelism) — Kuper and Newton.
- [Sustrik (2016)](https://scholar.google.com/scholar?q=Sustrik+2016+Structured+concurrency) — Sustrik, 2016.
- [Smith (2018)](https://scholar.google.com/scholar?q=Smith+2018+Notes+on+structured+concurrency+nurseries) — Smith, 2018.
- [Disentanglement in Nested-Parallel Programs](https://scholar.google.com/scholar?q=Disentanglement+in+Nested-Parallel+Programs) — Westrick, Yadav, Fluet and Acar, POPL 2020.
- [When Concurrency Matters](https://scholar.google.com/scholar?q=When+Concurrency+Matters+Behaviour-Oriented+Concurrency) — Cheeseman et al., OOPSLA 2023.
- [DisLog](https://scholar.google.com/scholar?q=DisLog+disentanglement+logic+Iris+Rocq+2024) — Iris and Rocq, 2024.
- [Data Race Freedom a la Mode](https://scholar.google.com/scholar?q=Data+Race+Freedom+a+la+Mode) — Georges, White, Dolan, Eisenberg, Casinghino, Pottier and Dreyer, POPL 2025.
- [TypeDis](https://scholar.google.com/scholar?q=TypeDis+static+disentanglement+type+system+2026) — 2026.

## Determinism, replay and content addressing

- [Gray and Reuter (1993)](https://scholar.google.com/scholar?q=Gray+Reuter+1993+Transaction+Processing+Concepts+and+Techniques) — Gray and Reuter, 1993.
- [A Critique of ANSI SQL Isolation Levels](https://scholar.google.com/scholar?q=A+Critique+of+ANSI+SQL+Isolation+Levels) — Berenson et al., SIGMOD 1995.
- [Dolstra (2006)](https://scholar.google.com/scholar?q=Dolstra+2006+The+Purely+Functional+Software+Deployment+Model) — Dolstra, 2006.
- [BLAKE3](https://scholar.google.com/scholar?q=BLAKE3+one+function+fast+everywhere) — O'Connor, Aumasson, Neves and Wilcox-O'Hearn, 2020.
- [Zhou et al. (2021)](https://scholar.google.com/scholar?q=Zhou+2021+FoundationDB+A+Distributed+Unbundled+Transactional+Key+Value+Store) — Zhou et al., SIGMOD 2021.
