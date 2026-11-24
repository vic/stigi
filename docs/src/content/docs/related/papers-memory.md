---
title: "Papers: memory and resources"
description: Ownership, regions, typestate, in-place updates, stack bounds and safe indexing.
sidebar:
  order: 12
---

Stigi frees memory at its last use, keeps stacks bounded and tracks resource states, all without a garbage collector. These papers show how ownership, regions and typestate make that safe.

## Ownership and borrowing

- [Schorr and Waite (1967)](https://scholar.google.com/scholar?q=Schorr+Waite+1967+An+efficient+machine-independent+procedure+for+garbage+collection) — Schorr and Waite, CACM 1967.
- [Checking Interference with Fractional Permissions](https://scholar.google.com/scholar?q=Checking+Interference+with+Fractional+Permissions) — Boyland, SAS 2003.
- [Matsakis (2018)](https://scholar.google.com/scholar?q=Matsakis+2018+Polonius+Rust+borrow+checker+Datalog) — Matsakis, 2018.
- [RustBelt](https://scholar.google.com/scholar?q=RustBelt+Securing+the+Foundations+of+the+Rust+Programming+Language) — Jung et al., POPL 2018.
- [Modelling Rust's Reference Ownership Analysis Declaratively in Datalog](https://scholar.google.com/scholar?q=Modelling+Rusts+Reference+Ownership+Analysis+Declaratively+in+Datalog) — Stjerna, 2020.
- [Oxidizing OCaml with Modal Memory Management](https://scholar.google.com/scholar?q=Oxidizing+OCaml+with+Modal+Memory+Management) — Lorenzen, White, Dolan, Eisenberg and Lindley, ICFP 2024.
- [Tree Borrows](https://scholar.google.com/scholar?q=Tree+Borrows+Villani+Hostert+Dreyer+Jung+PLDI+2025) — Villani, Hostert, Dreyer and Jung, PLDI 2025.

## Regions and reference counting

- [Region-Based Memory Management](https://scholar.google.com/scholar?q=Region-Based+Memory+Management+Tofte+Talpin) — Tofte and Talpin, Information and Computation 1997.
- [ASAP: As Static As Possible Memory Management](https://scholar.google.com/scholar?q=ASAP+As+Static+As+Possible+memory+management+Proust) — Proust, Cambridge TR 908, 2017.
- [Perceus: Garbage Free Reference Counting with Reuse](https://scholar.google.com/scholar?q=Perceus+Garbage+Free+Reference+Counting+with+Reuse) — Reinking, Xie, de Moura and Leijen, PLDI 2021.

## In-place updates and recursion

- [FP2: Fully in-Place Functional Programming](https://scholar.google.com/scholar?q=FP2+Fully+in-Place+Functional+Programming) — Lorenzen, Leijen and Swierstra, ICFP 2023.
- [Tail Recursion Modulo Context](https://scholar.google.com/scholar?q=Tail+Recursion+Modulo+Context+Leijen+Lorenzen) — Leijen and Lorenzen, PLDI 2023.
- [The Functional Essence of Imperative Binary Search Trees](https://scholar.google.com/scholar?q=The+Functional+Essence+of+Imperative+Binary+Search+Trees) — Lorenzen, Leijen, Swierstra and Lindley, PLDI 2024.

## Typestate

- [Typestate](https://scholar.google.com/scholar?q=Typestate+Strom+Yemini) — Strom and Yemini, IEEE TSE 1986.

## Stack, layout and indexing

- [Eliminating Stack Overflow by Abstract Interpretation](https://scholar.google.com/scholar?q=Eliminating+Stack+Overflow+by+Abstract+Interpretation) — Regehr, Reid and Webb, EMSOFT 2003.
- [Polyhedral tiling](https://scholar.google.com/scholar?q=Bondhugula+2008+A+practical+automatic+polyhedral+parallelizer+and+locality+optimizer+PLDI) — Bondhugula et al., PLDI 2008.
- [Hacker's Delight](https://scholar.google.com/scholar?q=Warren+Hackers+Delight+2nd+edition+2012) — Warren, 2nd ed. 2012.
- [Carbonneaux, Hoffmann, Ramananandro and Shao (2014)](https://scholar.google.com/scholar?q=Carbonneaux+Hoffmann+Ramananandro+Shao+2014+End-to-end+verification+of+stack-space+bounds+for+C+programs) — Carbonneaux, Hoffmann, Ramananandro and Shao, PLDI 2014.
- [Spectre Attacks: Exploiting Speculative Execution](https://scholar.google.com/scholar?q=Spectre+Attacks+Exploiting+Speculative+Execution) — Kocher et al., S&P 2019.
- [Blade](https://scholar.google.com/scholar?q=Blade+Vassena+POPL+2021+Automatically+eliminating+speculative+leaks+from+cryptographic+code) — Vassena et al., POPL 2021.
- [Ultimate SLH](https://scholar.google.com/scholar?q=Ultimate+SLH+Zhang+USENIX+Security+2023) — Zhang et al., USENIX Security 2023.
- [CuTe layout algebra](https://scholar.google.com/scholar?q=Thakkar+2023+CuTe+layout+algebra+CUTLASS) — Thakkar et al., 2023.
- [Blelloch](https://scholar.google.com/scholar?q=Blelloch+structured+partitioning+arrays+parallel) — Blelloch.
