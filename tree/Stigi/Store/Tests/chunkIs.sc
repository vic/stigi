// prompt: tell whether a run of the report buffer from a position holds exactly the expected bytes.
def Stigi.Store.Tests.chunkIs(report: Array[U8](65536), start: U64, size: U64, expected: Array[U8](size)) <lessOrEqual(start, 65536)>: Bool =
  var index: U64 = 0
  var at: U64 = start
  var same: Bool = true
  while index < size do
    @<lessOrEqual(index, size), lessOrEqual(start, at)>
    if at < 65536 then
      same = same && report(at) == expected(index)
      at = at + 1
    else
      same = false
    index = index + 1
  same
