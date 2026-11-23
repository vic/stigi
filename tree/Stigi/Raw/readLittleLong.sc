// prompt: decode a little-endian sixty-four-bit word from a proved byte slice.
pub extern "c" def Stigi.Raw.readLittleLong(cap: Stigi.ByteCap, bytes: Array[U8](cap), offset: U64) <below(offset, cap), below(offset + 1, cap), below(offset + 2, cap), below(offset + 3, cap), below(offset + 4, cap), below(offset + 5, cap), below(offset + 6, cap), below(offset + 7, cap)>: U64 =
  val lowWord: U64 = U64.ofU32(Stigi.Raw.readLittleWord(cap, bytes, offset))
  val byteFour: U64 = U64.shiftLeft(U64.ofU8(bytes(offset + 4)), 32)
  val byteFive: U64 = U64.shiftLeft(U64.ofU8(bytes(offset + 5)), 40)
  val byteSix: U64 = U64.shiftLeft(U64.ofU8(bytes(offset + 6)), 48)
  val byteSeven: U64 = U64.shiftLeft(U64.ofU8(bytes(offset + 7)), 56)
  U64.bitwiseOr(U64.bitwiseOr(lowWord, byteFour), U64.bitwiseOr(byteFive, U64.bitwiseOr(byteSix, byteSeven)))
