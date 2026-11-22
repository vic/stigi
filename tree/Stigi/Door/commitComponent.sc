// prompt: atomically commit a dependency-closed component or leave committed digests unchanged on rejection.
pub def Stigi.Door.commitComponent {Stigi.Mut} (unitCount: U64, committedLow: Array[U64](unitCount), candidateLow: Array[U64](unitCount), candidateCode: Array[U64](unitCount), candidateCached: Array[U8](unitCount)) <below(0, unitCount), below(unitCount, 4096)>: Stigi.Door.TransactionOutcome =
  var checkIndex: U64 = 0
  var failed: Bool = false
  var failSlot: U64 = 0
  var failCode: U64 = 0
  var cachedTotal: U64 = 0
  var computedTotal: U64 = 0
  while checkIndex < unitCount && !failed do
    @<lessOrEqual(checkIndex, unitCount), lessOrEqual(cachedTotal, checkIndex), lessOrEqual(computedTotal, checkIndex), decreases(unitCount - checkIndex)>
    val nextCheck: U64 = checkIndex + 1
    val codeVal: U64 = candidateCode(checkIndex)
    if codeVal != 0 then
      failed = true
      failSlot = checkIndex
      failCode = codeVal
    else
      val isCached: Bool = candidateCached(checkIndex) != 0
      cachedTotal = if isCached then cachedTotal + 1 else cachedTotal
      computedTotal = if !isCached then computedTotal + 1 else computedTotal
    checkIndex = nextCheck
  if failed then
    TransactionRejected(failSlot, failCode)
  else
    var commitIndex: U64 = 0
    while commitIndex < unitCount do
      @<lessOrEqual(commitIndex, unitCount), decreases(unitCount - commitIndex)>
      committedLow(commitIndex) := candidateLow(commitIndex)
      commitIndex = commitIndex + 1
    TransactionAdmitted(unitCount, cachedTotal, computedTotal)
