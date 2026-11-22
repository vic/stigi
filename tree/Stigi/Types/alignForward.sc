// prompt: round a byte size forward to the next multiple of alignment without overflow.
pub def Stigi.Types.alignForward(value: U64, align: U64) <below(value, 0x100000000), below(0, align), below(align, 4096)>: U64 <lessOrEqual(value, result)> =
  val rem: U64 = value % align
  if rem == 0 then
    value
  else
    val pad: U64 = align - rem
    value + pad
