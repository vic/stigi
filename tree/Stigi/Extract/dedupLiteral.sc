// prompt: intern an integer literal into a flat table and return its canonical slot or the updated count.
pub def Stigi.Extract.dedupLiteral {Stigi.Mut} (tableCap: U64, literals: Array[I64](tableCap), literalCount: U64, value: I64) <lessOrEqual(literalCount, tableCap), below(tableCap, 4096)>: U64 <lessOrEqual(literalCount, result), lessOrEqual(result, tableCap)> =
  var index: U64 = 0
  var found: Bool = false
  while index < literalCount && !found do
    @<lessOrEqual(index, literalCount), decreases(literalCount - index)>
    if literals(index) == value then
      found = true
      index = index + 1
    else
      index = index + 1
  if found then
    literalCount
  else if literalCount < tableCap then
    literals(literalCount) := value
    literalCount + 1
  else
    literalCount
