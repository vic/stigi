// prompt: pack one up-to-sixty-four-byte slice into sixteen little-endian message words.
def Stigi.Hash.loadBlockWords {region: Stigi.Region, Stigi.Mut} (cap: Stigi.ByteCap, data: Array[U8](cap), startPos: U64, byteLen: U64, outWords: Array[U32](16)) <lessOrEqual(byteLen, 64), lessOrEqual(startPos, cap)>: Bool =
  val Filled(padded) = Array.tryFill[U8]{region}(64, 0) else
    case FillFailed => false
  var idx: U64 = 0
  while idx < byteLen do
    @<lessOrEqual(idx, byteLen)>
    val srcPos: U64 = startPos + idx
    if srcPos < cap then padded(idx) := data(srcPos) else ()
    idx = idx + 1
  var wordIdx: U64 = 0
  while wordIdx < 16 do
    @<lessOrEqual(wordIdx, 16)>
    val bytePos: U64 = wordIdx * 4
    if bytePos < 61 then
      val posOne: U64 = bytePos + 1
      val posTwo: U64 = posOne + 1
      val posThree: U64 = posTwo + 1
      if posThree < 64 then
        val wordZero: U32 = U32.ofU64(U64.ofU8(padded(bytePos)))
        val wordOne: U32 = U32.shiftLeft(U32.ofU64(U64.ofU8(padded(posOne))), 8)
        val wordTwo: U32 = U32.shiftLeft(U32.ofU64(U64.ofU8(padded(posTwo))), 16)
        val wordThree: U32 = U32.shiftLeft(U32.ofU64(U64.ofU8(padded(posThree))), 24)
        outWords(wordIdx) := U32.bitwiseOr(U32.bitwiseOr(wordZero, wordOne), U32.bitwiseOr(wordTwo, wordThree))
      else ()
    else ()
    wordIdx = wordIdx + 1
  true
