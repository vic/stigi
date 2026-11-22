// prompt: prove component count never exceeds unit table capacity.
pub lemma def Stigi.Door.componentCountBounded(componentCount: U64, unitCount: U64, cap: U64) <lessOrEqual(componentCount, unitCount), lessOrEqual(unitCount, cap)>: <lessOrEqual(componentCount, cap)> =
  ()
