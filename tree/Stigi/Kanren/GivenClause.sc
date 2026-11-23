// prompt: normalized given clause entry for Bezem-bounded uKanren implicit resolution.
enum Stigi.Kanren.GivenClause:
  case GivenClause(traitTag: U64, argTag: U64, hasPrerequisite: Bool, reqTraitTag: U64, reqArgTag: U64, isEffectful: Bool)
