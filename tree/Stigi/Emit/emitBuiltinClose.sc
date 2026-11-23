// prompt: emit the closing C suffix for an active expression frame.
pub def Stigi.Emit.emitBuiltinClose {Stigi.Mut} (frameKind: U8, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  if frameKind == 1 then
    Stigi.Emit.writeLiteral(1, ")", outCap, output, writeOffset)
  else if frameKind == 2 then
    Stigi.Emit.writeLiteral(1, "]", outCap, output, writeOffset)
  else if frameKind == 10 then
    Stigi.Emit.writeLiteral(11, ") & 63ULL))", outCap, output, writeOffset)
  else if frameKind == 11 then
    Stigi.Emit.writeLiteral(3, ")))", outCap, output, writeOffset)
  else if frameKind == 12 then
    Stigi.Emit.writeLiteral(10, ") & 31U)))", outCap, output, writeOffset)
  else if frameKind == 14 then
    Stigi.Emit.writeLiteral(7, " } } })", outCap, output, writeOffset)
  else if frameKind == 15 || frameKind == 16 then
    Stigi.Emit.writeLiteral(6, "), 1))", outCap, output, writeOffset)
  else if frameKind == 17 then
    Stigi.Emit.writeLiteral(3, ")))", outCap, output, writeOffset)
  else
    Stigi.Emit.writeLiteral(2, "))", outCap, output, writeOffset)
