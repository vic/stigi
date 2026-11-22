// prompt: decode sixteen lowercase hexadecimal bytes starting at offset into one U64 word.
pub def Stigi.Store.decodeHexWord(cap: U64, text: Array[U8](cap), startOffset: U64) <lessOrEqual(startOffset, cap), below(cap, 0x100000000)>: U64 =
  var step: U64 = 0
  var accum: U64 = 0
  while step < 16 do
    @<lessOrEqual(step, 16), decreases(16 - step)>
    val bytePos: U64 = startOffset + step
    if bytePos < cap then
      val nib: U64 = Stigi.Store.hexDigitValue(text(bytePos))
      val cleanNib: U64 = if nib < 16 then nib else 0
      accum = U64.bitwiseOr(U64.shiftLeft(accum, 4), cleanNib)
    else
      ()
    step = step + 1
  accum
