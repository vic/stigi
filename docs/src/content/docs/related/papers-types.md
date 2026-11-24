---
title: "Papers: types and proofs"
description: Types, refinements, proofs and certificates that shape how Stigi checks programs.
sidebar:
  order: 10
---

Stigi checks programs with dependent array lengths and refinement facts, and its own soundness is checked outside of Stigi. These papers explain the ideas behind that.

## Dependent and refinement types

- [Coquand and Paulin-Mohring (1990)](https://scholar.google.com/scholar?q=Coquand+Paulin-Mohring+1990+inductively+defined+types) — Coquand and Paulin-Mohring, 1990.
- [Kennedy (1994)](https://scholar.google.com/scholar?q=Kennedy+1994+dimension+types+units+of+measure+ESOP) — Kennedy, ESOP 1994.
- [Eliminating Array Bound Checking Through Dependent Types](https://scholar.google.com/scholar?q=Eliminating+Array+Bound+Checking+Through+Dependent+Types) — Xi and Pfenning, PLDI 1998.
- [McBride (2016)](https://scholar.google.com/scholar?q=McBride+2016+I+got+plenty+o+nuttin) — McBride, 2016.
- [Atkey (2018)](https://scholar.google.com/scholar?q=Atkey+2018+Syntax+and+Semantics+of+Quantitative+Type+Theory+LICS) — Atkey, LICS 2018.
- [Liquid Haskell](https://scholar.google.com/scholar?q=Handley+Vazou+Hutton+2020+Liquid+Haskell+POPL) — Handley, Vazou and Hutton, POPL 2020.
- [Liquid types for Rust](https://scholar.google.com/scholar?q=Liquid+types+for+Rust+PLDI+2023) — PACMPL 7 (PLDI) 2023.
- [Theocharis and Brady (2026)](https://scholar.google.com/scholar?q=Theocharis+Brady+2026+erasure+phase+distinction+FSCD) — Theocharis and Brady, FSCD 2026.
- [Refinement types over inductive data](https://arxiv.org/abs/2611.20867) — Ferrarini, Vazou and Kavvos, extended POPL 2027.

## Type soundness and abstraction

- [Tait (1967)](https://scholar.google.com/scholar?q=Tait+1967+Intensional+interpretations+of+functionals+of+finite+type) — Tait, JSL 1967.
- [Lambda Calculus Notation with Nameless Dummies](https://scholar.google.com/scholar?q=Lambda+Calculus+Notation+with+Nameless+Dummies) — de Bruijn, 1972.
- [Morris (1973)](https://scholar.google.com/scholar?q=Morris+1973+Types+are+not+sets) — Morris, 1973.
- [Parametricity](https://scholar.google.com/scholar?q=Reynolds+1983+Types+abstraction+and+parametric+polymorphism) — Reynolds, 1983.
- [ADT Representation Independence](https://scholar.google.com/scholar?q=Mitchell+Plotkin+1988+Abstract+types+have+existential+type) — Mitchell and Plotkin, TOPLAS 1988.
- [Wadler and Blott (1989)](https://scholar.google.com/scholar?q=Wadler+Blott+1989+How+to+make+ad-hoc+polymorphism+less+ad+hoc) — Wadler and Blott, POPL 1989.
- [Proofs and Types](https://scholar.google.com/scholar?q=Girard+Lafont+Taylor+1989+Proofs+and+Types) — Girard, Lafont and Taylor, 1989.
- [A Syntactic Approach to Type Soundness](https://scholar.google.com/scholar?q=A+Syntactic+Approach+to+Type+Soundness) — Wright and Felleisen, Information and Computation 1994.
- [Coquand (1994)](https://scholar.google.com/scholar?q=Coquand+1994+Infinite+objects+in+type+theory+guarded) — Coquand, 1994.
- [Takahashi (1995)](https://scholar.google.com/scholar?q=Takahashi+Parallel+reductions+in+lambda-calculus) — Takahashi, Information and Computation 1995.
- [Hinze (2000)](https://scholar.google.com/scholar?q=Hinze+2000+Generic+programs+and+proofs+datatype-generic) — Hinze, 2000.
- [ITrees](https://scholar.google.com/scholar?q=Interaction+Trees+Xia+POPL+2020) — Xia et al., POPL 2020.

## Proofs, certificates and trust

- [Thompson (1984)](https://scholar.google.com/scholar?q=Thompson+1984+Reflections+on+Trusting+Trust) — Thompson, 1984.
- [Proof-Carrying Code](https://scholar.google.com/scholar?q=Necula+1997+Proof-Carrying+Code+POPL) — Necula, POPL 1997.
- [Pnueli, Siegel and Singerman (1998)](https://scholar.google.com/scholar?q=Pnueli+Siegel+Singerman+1998+Translation+validation) — Pnueli, Siegel and Singerman, 1998.
- [Countering Trusting Trust through Diverse Double-Compiling](https://scholar.google.com/scholar?q=Countering+Trusting+Trust+through+Diverse+Double-Compiling) — Wheeler, ACSAC 2005.
- [CompCert](https://scholar.google.com/scholar?q=Leroy+2009+Formal+verification+of+a+realistic+compiler+CompCert) — Leroy, 2009.
- [Kovacs (2020)](https://scholar.google.com/scholar?q=Kovacs+2020+normalization+by+evaluation+Lean+kernel) — Kovacs, 2020.
- [Lean4Lean](https://scholar.google.com/scholar?q=Carneiro+Lean4Lean+verified+Lean+kernel) — Carneiro, 2024-2025.
- [Cedar verification-guided development](https://scholar.google.com/scholar?q=Cutler+2024+Cedar+verification-guided+development+OOPSLA) — Cutler et al., OOPSLA 2024.
- [Verifying Datalog Reasoning with Lean](https://scholar.google.com/scholar?q=Verifying+Datalog+Reasoning+with+Lean) — Tantow, Gerlach, Mennicke and Krotzsch, ITP 2025.
- [PBLean](https://scholar.google.com/scholar?q=PBLean+VeriPB+certificates+in+Lean+4) — 2026.
- [Trying to make CompCert as fast as gcc -O2](https://ut-infraai.github.io) — ut-infraai.github.io, 2026.
- [Yang (Schorr-Waite in Coq)](https://scholar.google.com/scholar?q=Yang+Schorr-Waite+separation+logic+proof+Coq) — Yang, Coq.

## Verified systems code

- [RefinedC](https://scholar.google.com/scholar?q=RefinedC+Sammler+PLDI+2021+automating+the+foundational+verification+of+C+code) — Sammler et al., PLDI 2021.
- [CN: Verifying Systems C Code with Separation-Logic Refinement Types](https://scholar.google.com/scholar?q=CN+Verifying+Systems+C+Code+with+Separation-Logic+Refinement+Types) — Pulte, Makwana, Sewell, Memarian, Sewell and Krishnaswami, POPL 2023.

## Refinement, equivalence and information flow

- [Stepwise Program Refinement](https://scholar.google.com/scholar?q=Wirth+1971+Program+development+by+stepwise+refinement) — Wirth, 1971.
- [Milner (1971)](https://scholar.google.com/scholar?q=Milner+1971+An+algebraic+definition+of+simulation+between+programs) — Milner, 1971.
- [Hoare (1972)](https://scholar.google.com/scholar?q=Hoare+1972+Proof+of+correctness+of+data+representations) — Hoare, 1972.
- [Denning (1976)](https://scholar.google.com/scholar?q=Denning+1976+A+lattice+model+of+secure+information+flow) — Denning, 1976.
- [Hopcroft and Ullman (1979)](https://scholar.google.com/scholar?q=Hopcroft+Ullman+1979+Introduction+to+Automata+Theory+Languages+and+Computation) — Hopcroft and Ullman, 1979.
- [Milner (1980)](https://scholar.google.com/scholar?q=Milner+1980+A+Calculus+of+Communicating+Systems) — Milner, 1980.
- [Goguen and Meseguer (1982)](https://scholar.google.com/scholar?q=Goguen+Meseguer+1982+Security+policies+and+security+models) — Goguen and Meseguer, 1982.
- [Jones (1983)](https://scholar.google.com/scholar?q=Jones+1983+Tentative+steps+toward+a+development+method+for+interfering+programs) — Jones, 1983.
- [Paige and Tarjan (1987)](https://scholar.google.com/scholar?q=Paige+Tarjan+1987+Three+partition+refinement+algorithms) — Paige and Tarjan, 1987.
- [Abadi and Lamport (1991)](https://scholar.google.com/scholar?q=Abadi+Lamport+1991+The+existence+of+refinement+mappings) — Abadi and Lamport, 1991.
- [Liskov and Wing (1994)](https://scholar.google.com/scholar?q=Liskov+Wing+1994+A+behavioral+notion+of+subtyping) — Liskov and Wing, 1994.
- [Beer et al. (1997)](https://scholar.google.com/scholar?q=Beer+1997+Efficient+detection+of+vacuity+in+ACTL+formulas+CAV) — Beer et al., CAV 1997.
- [Refinement Calculus](https://scholar.google.com/scholar?q=Back+von+Wright+1998+Refinement+Calculus) — Back and von Wright, 1998.
- [Kupferman and Vardi (1999)](https://scholar.google.com/scholar?q=Kupferman+Vardi+1999+Vacuity+detection+in+temporal+model+checking) — Kupferman and Vardi, CAV 1999.
- [Interface Automata](https://scholar.google.com/scholar?q=de+Alfaro+Henzinger+2001+Interface+Automata) — de Alfaro and Henzinger, 2001.
- [Sabelfeld and Myers (2003)](https://scholar.google.com/scholar?q=Sabelfeld+Myers+2003+Language-based+information-flow+security) — Sabelfeld and Myers, JSAC 2003.
- [Giacobazzi and Mastroeni (2004)](https://scholar.google.com/scholar?q=Giacobazzi+Mastroeni+2004+Abstract+non-interference) — Giacobazzi and Mastroeni, 2004.

## Type-directed search and synthesis

- [A Machine-Oriented Logic Based on the Resolution Principle](https://scholar.google.com/scholar?q=A+Machine-Oriented+Logic+Based+on+the+Resolution+Principle) — Robinson, JACM 1965.
- [Rittri (1989)](https://scholar.google.com/scholar?q=Rittri+1989+Using+types+as+search+keys+in+function+libraries) — Rittri, FPCA 1989.
- [Di Cosmo (1995)](https://scholar.google.com/scholar?q=Di+Cosmo+1995+Isomorphisms+of+Types) — Di Cosmo, 1995.
- [Lau et al. (2003)](https://scholar.google.com/scholar?q=Lau+Domingos+Weld+2003+Programming+by+demonstration+version+space+algebra) — Lau et al., 2003.
- [Gulwani (2011)](https://scholar.google.com/scholar?q=Gulwani+2011+Automating+string+processing+in+spreadsheets+using+input-output+examples) — Gulwani, POPL 2011.
- [Hemann and Friedman (2013)](https://scholar.google.com/scholar?q=Hemann+Friedman+2013+microKanren+minimalist+language+for+relational+programming) — Hemann and Friedman, 2013.
- [Myth](https://scholar.google.com/scholar?q=Osera+Zdancewic+2015+Type-and-example-directed+program+synthesis) — Osera and Zdancewic, PLDI 2015.
- [Synquid](https://scholar.google.com/scholar?q=Program+Synthesis+from+Polymorphic+Refinement+Types+Synquid) — Polikarpova, Kuraj and Solar-Lezama, PLDI 2016.
- [Barliman](https://scholar.google.com/scholar?q=Byrd+2017+Barliman+trying+another+LISP+relational+program+synthesis) — Byrd et al., ICFP 2017.
- [Resyn](https://scholar.google.com/scholar?q=Resource-Guided+Program+Synthesis+Resyn) — Knoth, Wang, Polikarpova and Hoffmann, PLDI 2019.
- [Verse calculus](https://scholar.google.com/scholar?q=The+Verse+Calculus+a+core+calculus+for+deterministic+functional+logic+programming) — Augustsson et al., ICFP 2023.

## Normalization and language tools

- [Definitional Interpreters for Higher-Order Programming Languages](https://scholar.google.com/scholar?q=Definitional+Interpreters+for+Higher-Order+Programming+Languages) — Reynolds, 1972.
- [Statman (1979)](https://scholar.google.com/scholar?q=Statman+1979+The+typed+lambda-calculus+is+not+elementary+recursive) — Statman, TCS 1979.
- [Howard (1980)](https://scholar.google.com/scholar?q=Howard+1980+The+formulae-as-types+notion+of+construction) — Howard, 1980.
- [Interaction Nets](https://scholar.google.com/scholar?q=Lafont+1990+Interaction+Nets+POPL) — Lafont, POPL 1990.
- [Sabry and Felleisen (1993)](https://scholar.google.com/scholar?q=Sabry+Felleisen+1993+Reasoning+about+programs+in+continuation-passing+style) — Sabry and Felleisen, 1993.
- [Asperti and Mairson (1998)](https://scholar.google.com/scholar?q=Asperti+Mairson+1998+Parallel+beta+reduction+is+not+elementary+recursive) — Asperti and Mairson, POPL 1998.
- [Maranget (2007)](https://scholar.google.com/scholar?q=Maranget+2007+Warnings+for+pattern+matching+JFP) — Maranget, JFP 2007.
- [Maranget (2008)](https://scholar.google.com/scholar?q=Maranget+2008+Compiling+pattern+matching+to+good+decision+trees) — Maranget, ML 2008.
- [Delta-Nets](https://arxiv.org/abs/2505.20314) — Salvadori, 2025.
