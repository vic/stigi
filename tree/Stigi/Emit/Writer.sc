// prompt: emitter and canonical printer output cursor capability protocol.
pub capability Stigi.Emit.Writer:
  def putByte {Stigi.Mut} (cap: Stigi.EmitCap, buf: Array[U8](cap), byte: U8): Unit
  def putSlice {Stigi.Mut} (cap: Stigi.EmitCap, buf: Array[U8](cap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), startPos: U64, endPos: U64) <lessOrEqual(startPos, endPos), lessOrEqual(endPos, srcCap)>: Unit
  def nextTemp(): U64
  def writtenCount(): U64
  def didOverflow(): Bool
