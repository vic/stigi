// prompt: extract single-pass SSA order, dominator, arithmetic, index-read, and progress facts for a counted loop.
pub def Stigi.Extract.extractBoundsLoop {Stigi.Mut} (factCap: U64, factKinds: Array[U8](factCap), factLeft: Array[U64](factCap), factRight: Array[U64](factCap), factScope: Array[U64](factCap), literalCap: U64, literals: Array[I64](literalCap), loopLimit: I64) <below(0, factCap), below(factCap, 65536), below(4, literalCap), below(literalCap, 4096)>: Stigi.Extract.ExtractOutcome =
  if factCap < 7 then
    ExtractBudgetExceeded
  else
    var litCount: U64 = 0
    litCount = Stigi.Extract.dedupLiteral(literalCap, literals, litCount, 0)
    litCount = Stigi.Extract.dedupLiteral(literalCap, literals, litCount, 1)
    litCount = Stigi.Extract.dedupLiteral(literalCap, literals, litCount, loopLimit)
    litCount = Stigi.Extract.dedupLiteral(literalCap, literals, litCount, 0)
    factKinds(0) := Stigi.Extract.factKindTag(FactDominates)
    factLeft(0) := 1
    factRight(0) := 0
    factScope(0) := 0
    factKinds(1) := Stigi.Extract.factKindTag(FactDominates)
    factLeft(1) := 2
    factRight(1) := 1
    factScope(1) := 0
    factKinds(2) := Stigi.Extract.factKindTag(FactLessOrEqual)
    factLeft(2) := 0
    factRight(2) := 1
    factScope(2) := 1
    factKinds(3) := Stigi.Extract.factKindTag(FactBelow)
    factLeft(3) := 1
    factRight(3) := 2
    factScope(3) := 2
    factKinds(4) := Stigi.Extract.factKindTag(FactAddNode)
    factLeft(4) := 3
    factRight(4) := 1
    factScope(4) := 2
    factKinds(5) := Stigi.Extract.factKindTag(FactIndexRead)
    factLeft(5) := 1
    factRight(5) := 2
    factScope(5) := 2
    factKinds(6) := Stigi.Extract.factKindTag(FactDecreases)
    factLeft(6) := 2
    factRight(6) := 1
    factScope(6) := 2
    Extracted(7, litCount)
