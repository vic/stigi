// prompt: normalized given clause entry for Bezem-bounded uKanren implicit resolution.
enum Stigi.Kanren.GivenClause:
  case GivenClause(traitTag: U64, argTag: U64, need: Stigi.Kanren.Need, isEffectful: Bool)
