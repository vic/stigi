// prompt: decode one little-endian 64-bit unsigned word from a byte buffer at offset using native shifts.
def Stigi.Posix.readLittleEndianWord(bufCap: U64, buf: Array[U8](bufCap), offset: U64) <lessOrEqual(offset, bufCap), below(bufCap, 0x100000000)>: U64 =
  val posZero: U64 = offset
  val posOne: U64 = offset + 1
  val posTwo: U64 = offset + 2
  val posThree: U64 = offset + 3
  val posFour: U64 = offset + 4
  val posFive: U64 = offset + 5
  val posSix: U64 = offset + 6
  val posSeven: U64 = offset + 7
  val byteZero: U64 = if posZero < bufCap then U64.ofU8(buf(posZero)) else 0
  val byteOne: U64 = if posOne < bufCap then U64.shiftLeft(U64.ofU8(buf(posOne)), 8) else 0
  val byteTwo: U64 = if posTwo < bufCap then U64.shiftLeft(U64.ofU8(buf(posTwo)), 16) else 0
  val byteThree: U64 = if posThree < bufCap then U64.shiftLeft(U64.ofU8(buf(posThree)), 24) else 0
  val byteFour: U64 = if posFour < bufCap then U64.shiftLeft(U64.ofU8(buf(posFour)), 32) else 0
  val byteFive: U64 = if posFive < bufCap then U64.shiftLeft(U64.ofU8(buf(posFive)), 40) else 0
  val byteSix: U64 = if posSix < bufCap then U64.shiftLeft(U64.ofU8(buf(posSix)), 48) else 0
  val byteSeven: U64 = if posSeven < bufCap then U64.shiftLeft(U64.ofU8(buf(posSeven)), 56) else 0
  val lowHalf: U64 = U64.bitwiseOr(U64.bitwiseOr(byteZero, byteOne), U64.bitwiseOr(byteTwo, byteThree))
  val highHalf: U64 = U64.bitwiseOr(U64.bitwiseOr(byteFour, byteFive), U64.bitwiseOr(byteSix, byteSeven))
  U64.bitwiseOr(lowHalf, highHalf)
