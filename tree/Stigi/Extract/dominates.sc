// prompt: decide whether ancestorScope dominates queryScope in a flat parent-pointer dominator tree.
pub def Stigi.Extract.dominates(scopeCount: U64, parentOf: Array[U64](scopeCount), queryScope: U64, ancestorScope: U64) <below(0, scopeCount), below(scopeCount, 4096), below(queryScope, scopeCount), below(ancestorScope, scopeCount)>: Bool =
  if queryScope == ancestorScope then
    true
  else
    var stepCount: U64 = 0
    var cursor: U64 = queryScope
    var matched: Bool = false
    var stopped: Bool = false
    while stepCount < scopeCount && !matched && !stopped do
      @<lessOrEqual(stepCount, scopeCount), below(cursor, scopeCount), decreases(scopeCount - stepCount)>
      val parent: U64 = parentOf(cursor)
      if parent == ancestorScope then
        matched = true
      else if parent == cursor || parent >= scopeCount then
        stopped = true
      else
        cursor = parent
      stepCount = stepCount + 1
    matched
