// prompt: count the zero-based local tag index of a registered case within its owning enum.
def Stigi.Emit.countEnumLocalTag(caseCap: U64, caseEnumUnit: Array[U64](caseCap), targetCase: U64, targetEnum: U64) <below(targetCase, caseCap), below(caseCap, 4096)>: U64 <lessOrEqual(result, targetCase)> =
  var tagCount: U64 = 0
  var idx: U64 = 0
  while idx < targetCase do
    @<lessOrEqual(idx, targetCase), lessOrEqual(tagCount, idx), decreases(targetCase - idx)>
    val nextIdx: U64 = idx + 1
    if caseEnumUnit(idx) == targetEnum then
      tagCount = tagCount + 1
    else
      ()
    idx = nextIdx
  tagCount
