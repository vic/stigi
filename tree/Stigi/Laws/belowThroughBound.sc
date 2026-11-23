// prompt: law proving below holds for an item below a bound that is at most the top bound.
pub lemma def Stigi.Laws.belowThroughBound(item: U64, midBound: U64, topBound: U64) <below(item, midBound), lessOrEqual(midBound, topBound)>: <below(item, topBound)> =
  @<below(item, topBound)>
