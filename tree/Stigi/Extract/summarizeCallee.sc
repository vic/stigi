// prompt: build a per-unit callee summary from extracted columnar fact rows.
pub def Stigi.Extract.summarizeCallee(factCap: U64, factKinds: Array[U8](factCap), factScope: Array[U64](factCap), factCount: U64, loopCount: U64, hasTaintSource: Bool) <lessOrEqual(factCount, factCap), below(factCap, 65536), below(loopCount, 4096)>: Stigi.Extract.CalleeSummary =
  var index: U64 = 0
  var entryCount: U64 = 0
  var exitCount: U64 = 0
  var progressCount: U64 = 0
  var capTaint: Bool = hasTaintSource
  val tagBelow: U8 = Stigi.Extract.factKindTag(FactBelow)
  val tagLeq: U8 = Stigi.Extract.factKindTag(FactLessOrEqual)
  val tagCap: U8 = Stigi.Extract.factKindTag(FactCapabilityState)
  val tagDec: U8 = Stigi.Extract.factKindTag(FactDecreases)
  while index < factCount do
    @<lessOrEqual(index, factCount), lessOrEqual(entryCount, index), lessOrEqual(exitCount, index), lessOrEqual(progressCount, index), decreases(factCount - index)>
    val nextIndex: U64 = index + 1
    val kindByte: U8 = factKinds(index)
    val scopeNode: U64 = factScope(index)
    val isOrderFact: Bool = kindByte == tagBelow || kindByte == tagLeq
    entryCount = if isOrderFact && scopeNode == 0 then entryCount + 1 else entryCount
    exitCount = if isOrderFact && scopeNode == 2 then exitCount + 1 else exitCount
    capTaint = if kindByte == tagCap then true else capTaint
    progressCount = if kindByte == tagDec then progressCount + 1 else progressCount
    index = nextIndex
  Summary(entryCount, exitCount, capTaint, progressCount >= loopCount)
