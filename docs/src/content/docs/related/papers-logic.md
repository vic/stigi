---
title: "Papers: Datalog and analysis"
description: Datalog, dataflow, abstract interpretation, program logic, termination and cost bounds.
sidebar:
  order: 11
---

Every check in Stigi is a Datalog rule, and every rejection says why. These papers explain the evaluation, the abstract domains, the logic and the cost bounds behind that.

## Datalog semantics and evaluation

- [Robinson (1965)](https://scholar.google.com/scholar?q=Robinson+1965+A+Machine-Oriented+Logic+Based+on+the+Resolution+Principle) — Robinson, JACM 1965.
- [Tarjan (1972)](https://scholar.google.com/scholar?q=Tarjan+1972+Depth-first+search+and+linear+graph+algorithms) — Tarjan, 1972.
- [Logic for Improving Integrity Checking in Relational Data Bases](https://scholar.google.com/scholar?q=Logic+for+Improving+Integrity+Checking+in+Relational+Data+Bases) — Nicolas, Acta Informatica 1982.
- [Vardi (1982)](https://scholar.google.com/scholar?q=Vardi+1982+The+complexity+of+relational+query+languages) — Vardi, 1982.
- [Bancilhon (1986)](https://scholar.google.com/scholar?q=Bancilhon+1986+Naive+evaluation+of+recursively+defined+relations) — Bancilhon, 1986.
- [Bancilhon, Maier, Sagiv and Ullman (1986)](https://scholar.google.com/scholar?q=Bancilhon+Maier+Sagiv+Ullman+1986+Magic+sets+and+other+strange+ways+to+implement+logic+programs) — Bancilhon, Maier, Sagiv and Ullman, PODS 1986.
- [Immerman (1986)](https://scholar.google.com/scholar?q=Immerman+1986+Relational+queries+computable+in+polynomial+time) — Immerman, 1986.
- [Apt, Blair and Walker (1988)](https://scholar.google.com/scholar?q=Apt+Blair+Walker+1988+Towards+a+theory+of+declarative+knowledge) — Apt, Blair and Walker, 1988.
- [Van Gelder, Ross and Schlipf (1991)](https://scholar.google.com/scholar?q=Van+Gelder+Ross+Schlipf+1991+The+well-founded+semantics+for+general+logic+programs) — Van Gelder, Ross and Schlipf, JACM 1991.
- [On the power of magic](https://scholar.google.com/scholar?q=Beeri+Ramakrishnan+On+the+power+of+magic) — Beeri and Ramakrishnan, JLP 1991.
- [Balbin, Port, Ramamohanarao and Meenakshi (1991)](https://scholar.google.com/scholar?q=Balbin+Port+Ramamohanarao+Meenakshi+1991+Efficient+bottom-up+computation+of+queries+on+stratified+databases) — Balbin, Port, Ramamohanarao and Meenakshi, JLP 1991.
- [Monotonic aggregation in deductive databases](https://scholar.google.com/scholar?q=Monotonic+aggregation+in+deductive+databases) — Ross and Sagiv, PODS 1992.
- [Rondogiannis and Wadge](https://arxiv.org/abs/cs/0306017) — Rondogiannis and Wadge, arXiv cs/0306017.
- [Foundations of Databases](https://scholar.google.com/scholar?q=Abiteboul+Hull+Vianu+1995+Foundations+of+Databases) — Abiteboul, Hull and Vianu, 1995.
- [Chen and Warren (1996)](https://scholar.google.com/scholar?q=Chen+Warren+1996+Tabled+evaluation+with+delaying+for+general+logic+programs) — Chen and Warren, JACM 1996.
- [Fagin, Kolaitis, Miller and Popa (2005)](https://scholar.google.com/scholar?q=Fagin+Kolaitis+Miller+Popa+2005+Data+exchange+semantics+and+query+answering) — Fagin, Kolaitis, Miller and Popa, TCS 2005.
- [Bounded term-size tabling](https://arxiv.org/abs/1210.2282) — TPLP, arXiv 1210.2282.
- [From Datalog to Flix](https://scholar.google.com/scholar?q=From+Datalog+to+Flix+a+declarative+language+for+fixed+points+on+lattices) — Madsen, Yee and Lhotak, PLDI 2016.
- [Jordan, Scholz and Subotic (2016)](https://scholar.google.com/scholar?q=Jordan+Scholz+Subotic+2016+Souffle+on+synthesis+of+program+analyzers) — Jordan, Scholz and Subotic, CAV 2016.
- [Fixpoint semantics and optimization of recursive Datalog programs with aggregates](https://scholar.google.com/scholar?q=Fixpoint+semantics+and+optimization+of+recursive+Datalog+programs+with+aggregates) — Zaniolo, Yang, Das, Shkapsky, Condie and Interlandi, TPLP 2017.
- [Abo Khamis, Ngo, Pichler, Suciu and Wang (2022)](https://scholar.google.com/scholar?q=Abo+Khamis+Ngo+Pichler+Suciu+Wang+2022+Convergence+of+datalog+over+pre-semirings) — Abo Khamis, Ngo, Pichler, Suciu and Wang, PODS 2022.
- [Free Join: Unifying Worst-Case Optimal and Traditional Joins](https://scholar.google.com/scholar?q=Free+Join+Unifying+Worst-Case+Optimal+and+Traditional+Joins) — Wang, Willsey and Suciu, SIGMOD 2023.
- [egglog](https://scholar.google.com/scholar?q=Better+Together+Unifying+Datalog+and+Equality+Saturation+egglog) — Zhang et al., PLDI 2023.
- [Budiu et al. (2023)](https://scholar.google.com/scholar?q=Budiu+2023+DBSP+automatic+incremental+view+maintenance+PVLDB) — Budiu et al., PVLDB 2023.
- [Suciu, Wang and Zhang (2025)](https://scholar.google.com/scholar?q=Suciu+Wang+Zhang+2025+equality+saturation+termination+ICDT) — Suciu, Wang and Zhang, ICDT 2025.

## Provenance and why-not answers

- [Eiter, Gottlob and Leone (1997)](https://scholar.google.com/scholar?q=Eiter+Gottlob+Leone+1997+Semantic+methods+for+abduction+complexity) — Eiter, Gottlob and Leone, 1997.
- [Green et al. (2007)](https://scholar.google.com/scholar?q=Green+Karvounarakis+Tannen+2007+Provenance+semirings+PODS) — Green et al., PODS 2007.
- [Huang et al. (2008)](https://scholar.google.com/scholar?q=Huang+Chen+Doan+Naughton+2008+On+the+provenance+of+non-answers+to+queries+over+extracted+data) — Huang et al., SIGMOD 2008.
- [Chapman and Jagadish (2009)](https://scholar.google.com/scholar?q=Chapman+Jagadish+2009+Why+not+SIGMOD) — Chapman and Jagadish, SIGMOD 2009.
- [Herschel and Hernandez (2010)](https://scholar.google.com/scholar?q=Herschel+Hernandez+2010+Explaining+missing+answers+to+SPJUA+queries) — Herschel and Hernandez, 2010.
- [Debugging Large-Scale Datalog](https://scholar.google.com/scholar?q=Debugging+Large-Scale+Datalog+A+Scalable+Provenance+Evaluation+Strategy) — Zhao, Subotic and Scholz, TOPLAS 2020.

## Dataflow and abstract interpretation

- [Futamura (1971)](https://scholar.google.com/scholar?q=Futamura+1971+Partial+evaluation+of+computation+process) — Futamura, 1971.
- [Kildall (1973)](https://scholar.google.com/scholar?q=Kildall+1973+A+unified+approach+to+global+program+optimization) — Kildall, 1973.
- [Tarjan (1974)](https://scholar.google.com/scholar?q=Tarjan+1974+Finding+dominators+in+directed+graphs) — Tarjan, 1974.
- [Cousot and Cousot (1977)](https://scholar.google.com/scholar?q=Cousot+Cousot+1977+Abstract+interpretation+a+unified+lattice+model) — Cousot and Cousot, 1977.
- [Kam and Ullman (1977)](https://scholar.google.com/scholar?q=Kam+Ullman+1977+Monotone+data+flow+analysis+frameworks) — Kam and Ullman, 1977.
- [Pratt (1977)](https://scholar.google.com/scholar?q=Pratt+1977+Two+easy+theories+whose+combination+is+hard) — Pratt, 1977.
- [Lengauer and Tarjan (1979)](https://scholar.google.com/scholar?q=Lengauer+Tarjan+1979+A+fast+algorithm+for+finding+dominators+in+a+flowgraph) — Lengauer and Tarjan, 1979.
- [Maintaining order in a linked list](https://scholar.google.com/scholar?q=Dietz+1982+Maintaining+order+in+a+linked+list) — Dietz, 1982.
- [Dill (1989)](https://scholar.google.com/scholar?q=Dill+1989+Timing+assumptions+and+verification+of+finite-state+concurrent+systems) — Dill, 1989.
- [Efficiently Computing Static Single Assignment Form](https://scholar.google.com/scholar?q=Efficiently+Computing+Static+Single+Assignment+Form+and+the+Control+Dependence+Graph) — Cytron et al., TOPLAS 1991.
- [Automatic construction of sparse data flow evaluation graphs](https://scholar.google.com/scholar?q=Automatic+construction+of+sparse+data+flow+evaluation+graphs) — Choi, Cytron and Ferrante, POPL 1991.
- [Baxter et al. (1998)](https://scholar.google.com/scholar?q=Baxter+1998+Clone+detection+using+abstract+syntax+trees+ICSM) — Baxter et al., ICSM 1998.
- [ABCD: Eliminating Array Bounds Checks on Demand](https://scholar.google.com/scholar?q=ABCD+Eliminating+Array+Bounds+Checks+on+Demand) — Bodik, Gupta and Sarkar, PLDI 2000.
- [The Octagon Abstract Domain](https://scholar.google.com/scholar?q=The+Octagon+Abstract+Domain) — Mine, HOSC 2006.
- [Pentagons](https://scholar.google.com/scholar?q=Pentagons+A+Weakly+Relational+Abstract+Domain+for+the+Efficient+Validation+of+Array+Accesses) — Logozzo and Fahndrich, APLAS 2008 and SCP 2010.

## Program logic and traces

- [Assigning Meanings to Programs](https://scholar.google.com/scholar?q=Floyd+1967+Assigning+Meanings+to+Programs) — Floyd, 1967.
- [An Axiomatic Basis for Computer Programming](https://scholar.google.com/scholar?q=Hoare+1969+An+Axiomatic+Basis+for+Computer+Programming) — Hoare, CACM 1969.
- [Pnueli (1977)](https://scholar.google.com/scholar?q=Pnueli+1977+The+temporal+logic+of+programs) — Pnueli, 1977.
- [Pearl (2000)](https://scholar.google.com/scholar?q=Pearl+2000+Causality+models+reasoning+and+inference) — Pearl, 2000.
- [Havelund and Rosu (2001)](https://scholar.google.com/scholar?q=Havelund+Rosu+2001+Monitoring+programs+using+rewriting) — Havelund and Rosu, RV 2001.
- [Separation Logic: A Logic for Shared Mutable Data Structures](https://scholar.google.com/scholar?q=Separation+Logic+A+Logic+for+Shared+Mutable+Data+Structures) — Reynolds, LICS 2002.
- [Wilhelm et al. (2008)](https://scholar.google.com/scholar?q=Wilhelm+2008+The+worst-case+execution-time+problem+overview+of+methods+and+survey+of+tools) — Wilhelm et al., TECS 2008.

## Termination

- [Dershowitz and Manna (1979)](https://scholar.google.com/scholar?q=Dershowitz+Manna+1979+Proving+termination+with+multiset+orderings) — Dershowitz and Manna, 1979.
- [Bezem (1993)](https://scholar.google.com/scholar?q=Bezem+1993+Strong+termination+of+logic+programs) — Bezem, JLP 1993.
- [Modular Termination Proofs for Logic and Pure PROLOG Programs](https://scholar.google.com/scholar?q=Modular+Termination+Proofs+for+Logic+and+Pure+PROLOG+Programs) — Apt and Pedreschi, 1993.
- [Akra and Bazzi (1998)](https://scholar.google.com/scholar?q=Akra+Bazzi+1998+On+the+solution+of+linear+recurrence+equations) — Akra and Bazzi, 1998.
- [The Size-Change Principle for Program Termination](https://scholar.google.com/scholar?q=The+Size-Change+Principle+for+Program+Termination) — Lee, Jones and Ben-Amram, POPL 2001.

## Cost and resource bounds

- [Tarjan (1985)](https://scholar.google.com/scholar?q=Tarjan+1985+Amortized+computational+complexity+SIAM) — Tarjan, SIAM 1985.
- [Crary and Weirich (2000)](https://scholar.google.com/scholar?q=Crary+Weirich+2000+Resource+bound+certification+POPL) — Crary and Weirich, POPL 2000.
- [Hofmann and Jost (2003)](https://scholar.google.com/scholar?q=Hofmann+Jost+2003+Static+prediction+of+heap+space+usage+for+first-order+functional+programs) — Hofmann and Jost, POPL 2003.
- [Hofmann and Jost (2006)](https://scholar.google.com/scholar?q=Hofmann+Jost+2006+Type-based+amortised+heap-space+analysis+ESOP) — Hofmann and Jost, ESOP 2006.
- [Ben-Amram, Jones and Kristiansen (2008)](https://scholar.google.com/scholar?q=Ben-Amram+Jones+Kristiansen+2008+Linear+polynomial+and+exponential+growth+bounds+mwp) — Ben-Amram, Jones and Kristiansen, CiE 2008 and TOPLAS 2009.
- [Hoffmann, Aehlig and Hofmann (2012)](https://scholar.google.com/scholar?q=Hoffmann+Aehlig+Hofmann+2012+Multivariate+amortized+resource+analysis) — Hoffmann, Aehlig and Hofmann, TOPLAS 2012.
- [Wang, Wang and Chlipala (2017)](https://scholar.google.com/scholar?q=Wang+Wang+Chlipala+2017+TiML+a+functional+language+for+practical+complexity+analysis) — Wang, Wang and Chlipala, OOPSLA 2017.
- [Swan and Pitts](https://arxiv.org/abs/2105.03252) — arXiv 2105.03252.
- [Atkey (2024)](https://scholar.google.com/scholar?q=Atkey+2024+Polynomial+time+and+dependent+types+POPL) — Atkey, POPL 2024.
- [Resource-Bounded Type Theory](https://arxiv.org/abs/2512.06952) — preprint, arXiv 2512.06952.
- [Glover and Hoffmann (2026)](https://scholar.google.com/scholar?q=Glover+Hoffmann+2026+resource-bounded+time+types+LICS) — Glover and Hoffmann, LICS 2026.
