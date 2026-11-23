// prompt: round a byte offset up to a proved alignment boundary without overflow.
pub def Stigi.Types.alignOffset(offset: U64, alignBytes: U64) <below(offset, Stigi.Limit.maxSourceBytes), below(0, alignBytes), lessOrEqual(alignBytes, 64)>: U64 <lessOrEqual(offset, result)> =
  val rem: U64 = offset % alignBytes
  if rem == 0 then offset else offset + (alignBytes - rem)
