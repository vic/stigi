// prompt: append a byte slice repeatedly into a buffer and return the updated cursor or overflow.
def Stigi.Syntax.Print.Tests.repeatText {Stigi.Mut} (bufferCap: U64, sourceBuffer: Array[U8](bufferCap), written: U64, sliceCap: U64, slice: Array[U8](sliceCap), count: U64) <lessOrEqual(written, bufferCap), lessOrEqual(bufferCap, Stigi.Limit.maxUnitBytes), lessOrEqual(sliceCap, bufferCap)>: U64 =
  var cursor: U64 = written
  var step: U64 = 0
  while step < count && sliceCap <= bufferCap - cursor do
    @<lessOrEqual(step, count), lessOrEqual(cursor, bufferCap)>
    cursor = Stigi.Array.copy[U8](sliceCap, slice, 0, sliceCap, bufferCap, sourceBuffer, cursor)
    step = step + 1
  if step < count then bufferCap + 1 else cursor
