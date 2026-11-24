// prompt: lower the or-alternatives of one pattern from one position, one decision arm each, the decreasing count bounding the recursion.
def Stigi.Elaborate.lowerAlternatives {Stigi.Mut} (tables: Stigi.Syntax.Tables, alternatives: Stigi.Syntax.Span, position: U64, bodyAt: U64, core: Stigi.Core.Tables, scratch: Stigi.Elaborate.Scratch, remaining: U64) <decreases(remaining)>: Stigi.Elaborate.CoreResult =
  alternatives match
    case Span(_, altEnd) =>
      if remaining < 1 || altEnd <= position then CoreDone(core) else
        @<below(position, Stigi.Limit.maxUnitNodes)>
        val Tables(_, _, _, synLists, _, _) = tables
        val altSyn: Stigi.Syntax.NodeAt = synLists(position)
        Stigi.Elaborate.lowerAlt(tables, altSyn, bodyAt, core, scratch) match
          case CoreFull => CoreFull
          case CoreInvalid => CoreInvalid
          case CoreDone(next) => Stigi.Elaborate.lowerAlternatives(tables, alternatives, position + 1, bodyAt, next, scratch, remaining - 1)
