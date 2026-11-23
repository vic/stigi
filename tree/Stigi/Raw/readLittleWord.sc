// prompt: decode a little-endian thirty-two-bit word from a proved byte slice.
pub extern "c" def Stigi.Raw.readLittleWord(cap: Stigi.ByteCap, bytes: Array[U8](cap), offset: U64) <below(offset, cap), below(offset + 1, cap), below(offset + 2, cap), below(offset + 3, cap)>: U32 =
  val byteZero: U32 = U32.ofU64(U64.ofU8(bytes(offset)))
  val byteOne: U32 = U32.shiftLeft(U32.ofU64(U64.ofU8(bytes(offset + 1))), 8)
  val byteTwo: U32 = U32.shiftLeft(U32.ofU64(U64.ofU8(bytes(offset + 2))), 16)
  val byteThree: U32 = U32.shiftLeft(U32.ofU64(U64.ofU8(bytes(offset + 3))), 24)
  U32.bitwiseOr(U32.bitwiseOr(byteZero, byteOne), U32.bitwiseOr(byteTwo, byteThree))
