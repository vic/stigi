// prompt: iteratively follow parent links in a disjoint-set forest to the canonical component root.
def Stigi.Door.findRoot(unitCount: U64, parentOf: Array[U64](unitCount), nodeIndex: U64) <below(0, unitCount), below(unitCount, 4096), below(nodeIndex, unitCount)>: U64 <below(result, unitCount)> =
  var cursor: U64 = nodeIndex
  var stepCount: U64 = 0
  var done: Bool = false
  while stepCount < unitCount && !done do
    @<lessOrEqual(stepCount, unitCount), below(cursor, unitCount), decreases(unitCount - stepCount)>
    val parent: U64 = parentOf(cursor)
    if parent == cursor || parent >= unitCount then
      done = true
    else
      cursor = parent
    stepCount = stepCount + 1
  cursor
