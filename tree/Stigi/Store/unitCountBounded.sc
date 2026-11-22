// prompt: prove parsed stored unit count stays bounded by table capacity.
pub lemma def Stigi.Store.unitCountBounded(unitCount: U64, maxUnits: U64, limit: U64) <lessOrEqual(unitCount, maxUnits), lessOrEqual(maxUnits, limit)>: <lessOrEqual(unitCount, limit)> =
  ()
