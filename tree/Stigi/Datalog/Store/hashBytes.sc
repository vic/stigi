// prompt: hash a byte slice using splitmix64 mixing.
def Stigi.Datalog.Store.hashBytes(srcCap: U64, src: Array[U8](srcCap), first: U64, endPos: U64) <lessOrEqual(first, endPos), lessOrEqual(endPos, srcCap)>: U64 =
  var acc: U64 = 1469598103934665603
  var idx: U64 = first
  while idx < endPos do
    val nextIdx: U64 = idx + 1
    val byteVal: U64 = U64.ofU8(src(idx))
    acc = U64.mix(U64.bitwiseXor(acc, byteVal))
    idx = nextIdx
  acc
