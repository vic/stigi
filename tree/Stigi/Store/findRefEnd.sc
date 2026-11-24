// prompt: scan an identifier or dotted path reference end offset in source bytes, stripping trailing dots.
def Stigi.Store.findRefEnd(synSource: Array[U8](Stigi.Limit.maxUnitBytes), refFirst: U64) <lessOrEqual(refFirst, Stigi.Limit.maxUnitBytes)>: U64 <lessOrEqual(refFirst, result), lessOrEqual(result, Stigi.Limit.maxUnitBytes)> =
  var pos: U64 = refFirst
  var lastValid: U64 = refFirst
  while pos < Stigi.Limit.maxUnitBytes && synSource(pos) is 'a'..'z' | 'A'..'Z' | '0'..'9' | '_' | '.' do
    @<lessOrEqual(refFirst, lastValid), lessOrEqual(lastValid, pos), lessOrEqual(pos, Stigi.Limit.maxUnitBytes)>
    val byteVal: U8 = synSource(pos)
    pos = pos + 1
    if byteVal != '.' then lastValid = pos else ()
  lastValid
