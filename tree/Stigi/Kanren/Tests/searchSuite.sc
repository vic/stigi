// prompt: verify Bezem-bounded uKanren search finds chained givens and rejects missing, ambiguous, cyclic, and effectful givens.
val Stigi.Kanren.Tests.searchSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Filled(clauses) = Array.tryFill[Stigi.Kanren.GivenClause]{Stigi.Region}(8, GivenClause(0, 0, false, 0, 0, false)) else
      case FillFailed => false
    clauses(0) := GivenClause(1, 10, false, 0, 0, false)
    clauses(1) := GivenClause(1, 20, true, 1, 10, false)
    clauses(2) := GivenClause(2, 30, false, 0, 0, false)
    clauses(3) := GivenClause(2, 30, false, 0, 0, false)
    clauses(4) := GivenClause(3, 40, true, 3, 40, false)
    clauses(5) := GivenClause(4, 50, false, 0, 0, true)
    val hitChain: Bool = Stigi.Kanren.resolveGoal(8, clauses, 6, 1, 20, 16) match
      case KanrenFound(slot) => slot == 1
      case KanrenMissing => false
      case KanrenAmbiguous => false
      case KanrenOutOfFuel => false
      case KanrenEffectful(_) => false
    val hitMiss: Bool = Stigi.Kanren.resolveGoal(8, clauses, 6, 9, 99, 16) match
      case KanrenMissing => true
      case KanrenFound(_) => false
      case KanrenAmbiguous => false
      case KanrenOutOfFuel => false
      case KanrenEffectful(_) => false
    val hitAmb: Bool = Stigi.Kanren.resolveGoal(8, clauses, 6, 2, 30, 16) match
      case KanrenAmbiguous => true
      case KanrenFound(_) => false
      case KanrenMissing => false
      case KanrenOutOfFuel => false
      case KanrenEffectful(_) => false
    val hitFuel: Bool = Stigi.Kanren.resolveGoal(8, clauses, 6, 3, 40, 8) match
      case KanrenOutOfFuel => true
      case KanrenFound(_) => false
      case KanrenMissing => false
      case KanrenAmbiguous => false
      case KanrenEffectful(_) => false
    val hitEff: Bool = Stigi.Kanren.resolveGoal(8, clauses, 6, 4, 50, 16) match
      case KanrenEffectful(slot) => slot == 5
      case KanrenFound(_) => false
      case KanrenMissing => false
      case KanrenAmbiguous => false
      case KanrenOutOfFuel => false
    hitChain && hitMiss && hitAmb && hitFuel && hitEff
  Stigi.Test.expect(passed, true, "uKanren search must verify chained, missing, ambiguous, cyclic, and effectful givens")
