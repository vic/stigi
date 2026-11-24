// prompt: pack one up-to-sixty-four-byte slice into the sixteen little-endian words of the work message.
def Stigi.Hash.loadBlockWords {Stigi.Mut} (cap: Stigi.ByteCap, data: Array[U8](cap), startPos: U64, byteLen: U64, work: Stigi.Hash.Work) <lessOrEqual(byteLen, 64), lessOrEqual(startPos, cap)>: Unit =
  val Work(_, block, _, _, _, _, padded) = work
  val endPos: U64 = startPos + byteLen
  val filled: U64 = if endPos <= cap then Stigi.Array.copy(cap, data, startPos, endPos, 64, padded, 0) else 0
  Stigi.Hash.zeroBytes(padded, filled)
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
        block(wordIdx) := U32.bitwiseOr(U32.bitwiseOr(wordZero, wordOne), U32.bitwiseOr(wordTwo, wordThree))
      else ()
    else ()
    wordIdx = wordIdx + 1
