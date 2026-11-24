// prompt: set every padded block byte from a position to the end to zero.
def Stigi.Hash.zeroBytes {Stigi.Mut} (padded: Array[U8](64), from: U64) <lessOrEqual(from, 64)>: Unit =
  var idx: U64 = from
  while idx < 64 do
    @<lessOrEqual(idx, 64)>
    padded(idx) := 0
    idx = idx + 1
