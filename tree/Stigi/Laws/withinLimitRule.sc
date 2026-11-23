// prompt: law proving withinLimit holds transitively through a proved bound.
pub lemma def Stigi.Laws.withinLimitRule(item: U64, midBound: U64, topBound: U64) <below(item, midBound), lessOrEqual(midBound, topBound)>: <Stigi.Rules.withinLimit(item, topBound)> =
  @<Stigi.Rules.withinLimit(item, topBound)>
