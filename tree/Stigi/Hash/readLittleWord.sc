// prompt: read four little-endian bytes from an input buffer with zero padding past endOffset.
def Stigi.Hash.readLittleWord(capacity: U64, bytes: Array[U8](capacity), offset: U64, endOffset: U64) <lessOrEqual(offset, endOffset), lessOrEqual(endOffset, capacity), below(capacity, 0x100000000)>: U32 =
  val posZero: U64 = offset
  val posOne: U64 = posZero + 1
  val posTwo: U64 = posOne + 1
  val posThree: U64 = posTwo + 1
  val byteZero: U64 = if posZero < endOffset then U64.ofU8(bytes(posZero)) else 0
  val byteOne: U64 = if posOne < endOffset then U64.ofU8(bytes(posOne)) else 0
  val byteTwo: U64 = if posTwo < endOffset then U64.ofU8(bytes(posTwo)) else 0
  val byteThree: U64 = if posThree < endOffset then U64.ofU8(bytes(posThree)) else 0
  val lowPair: U64 = U64.bitwiseOr(byteZero, U64.shiftLeft(byteOne, 8))
  val highPair: U64 = U64.bitwiseOr(U64.shiftLeft(byteTwo, 16), U64.shiftLeft(byteThree, 24))
  U32.ofU64(U64.bitwiseAnd(U64.bitwiseOr(lowPair, highPair), 0xffffffff))
