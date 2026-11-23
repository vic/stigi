// prompt: stack-allocated cursor handler implementing Stigi.Emit.Writer without box arrays.
pub handler Stigi.Emit.BufferWriter(capLimit: Stigi.EmitCap) for Stigi.Emit.Writer:
  var cursor: U64 = 0
  var tempSeq: U64 = 0
  var overflowed: Bool = false
  def putByte {Stigi.Mut} (cap: Stigi.EmitCap, buf: Array[U8](cap), byte: U8): Unit =
    val curSlot: U64 = cursor
    if curSlot < cap && curSlot < capLimit then
      buf(curSlot) := byte
      cursor = curSlot + 1
      ()
    else
      overflowed = true
      ()
  def putSlice {Stigi.Mut} (cap: Stigi.EmitCap, buf: Array[U8](cap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), startPos: U64, endPos: U64) <lessOrEqual(startPos, endPos), lessOrEqual(endPos, srcCap)>: Unit =
    var srcIdx: U64 = startPos
    while srcIdx < endPos && !overflowed do
      @<lessOrEqual(srcIdx, endPos)>
      val curSlot: U64 = cursor
      if curSlot < cap && curSlot < capLimit then
        buf(curSlot) := src(srcIdx)
        cursor = curSlot + 1
      else
        overflowed = true
      srcIdx = srcIdx + 1
    ()
  def nextTemp(): U64 =
    val curTemp: U64 = tempSeq
    if curTemp < Stigi.Limit.maxNodes then
      tempSeq = curTemp + 1
      curTemp
    else curTemp
  def writtenCount(): U64 = cursor
  def didOverflow(): Bool = overflowed
