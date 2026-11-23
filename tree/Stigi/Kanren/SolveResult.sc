// prompt: closed result of Bezem-bounded uKanren relational implicit resolution.
enum Stigi.Kanren.SolveResult:
  case KanrenFound(givenIndex: U64)
  case KanrenMissing
  case KanrenAmbiguous
  case KanrenOutOfFuel
  case KanrenEffectful(givenIndex: U64)
