// prompt: append a fixed-size byte slice to a buffer if it fits, or return overflow sentinel.
def Stigi.Syntax.Print.Tests.appendText {Stigi.Mut} (bufferCap: U64, sourceBuffer: Array[U8](bufferCap), written: U64, sliceCap: U64, slice: Array[U8](sliceCap)) <lessOrEqual(bufferCap, Stigi.Limit.maxUnitBytes), lessOrEqual(sliceCap, bufferCap)>: U64 =
  if written <= bufferCap && sliceCap <= bufferCap - written then Stigi.Array.copy[U8](sliceCap, slice, 0, sliceCap, bufferCap, sourceBuffer, written) else bufferCap + 1
