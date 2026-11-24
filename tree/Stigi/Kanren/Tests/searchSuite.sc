// prompt: verify Bezem-bounded uKanren search finds chained givens and rejects missing, ambiguous, cyclic, and effectful givens.
val Stigi.Kanren.Tests.searchSuite {Stigi.Test}: Stigi.Test.Result =
  val passed: Bool =
    val Filled(clauses) = Array.tryFill[Stigi.Kanren.GivenClause]{Stigi.Region}(8, GivenClause(0, 0, Ground, false)) else
      case FillFailed => false
    clauses(0) := GivenClause(1, 10, Ground, false)
    clauses(1) := GivenClause(1, 20, Requires(1, 10), false)
    clauses(2) := GivenClause(2, 30, Ground, false)
    clauses(3) := GivenClause(2, 30, Ground, false)
    clauses(4) := GivenClause(3, 40, Requires(3, 40), false)
    clauses(5) := GivenClause(4, 50, Ground, true)
    val hitChain: Bool = Stigi.Kanren.resolveGoal(8, clauses, 6, 1, 20, 16) is KanrenFound(1)
    val hitMiss: Bool = Stigi.Kanren.resolveGoal(8, clauses, 6, 9, 99, 16) is KanrenMissing
    val hitAmb: Bool = Stigi.Kanren.resolveGoal(8, clauses, 6, 2, 30, 16) is KanrenAmbiguous
    val hitFuel: Bool = Stigi.Kanren.resolveGoal(8, clauses, 6, 3, 40, 8) is KanrenOutOfFuel
    val hitEff: Bool = Stigi.Kanren.resolveGoal(8, clauses, 6, 4, 50, 16) is KanrenEffectful(5)
    hitChain && hitMiss && hitAmb && hitFuel && hitEff
  Stigi.Test.expect(passed, true, "uKanren search must verify chained, missing, ambiguous, cyclic, and effectful givens")
