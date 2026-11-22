// prompt: write one 32-bit digest word as four little-endian bytes into an output buffer.
def Stigi.Hash.writeLittleWord {Stigi.Mut} (word: U32, outCap: U64, output: Array[U8](outCap), offset: U64) <lessOrEqual(offset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(offset, result), lessOrEqual(result, outCap)> =
  val wide: U64 = U64.ofU32(word)
  val posZero: U64 = offset
  val posOne: U64 = posZero + 1
  val posTwo: U64 = posOne + 1
  val posThree: U64 = posTwo + 1
  if posThree < outCap then
    output(posZero) := U8.ofU64(U64.bitwiseAnd(wide, 255))
    output(posOne) := U8.ofU64(U64.bitwiseAnd(U64.shiftRight(wide, 8), 255))
    output(posTwo) := U8.ofU64(U64.bitwiseAnd(U64.shiftRight(wide, 16), 255))
    output(posThree) := U8.ofU64(U64.bitwiseAnd(U64.shiftRight(wide, 24), 255))
    posThree + 1
  else
    offset
