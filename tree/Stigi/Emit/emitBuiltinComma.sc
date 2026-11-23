// prompt: emit the C argument separator for an active expression frame.
pub def Stigi.Emit.emitBuiltinComma {Stigi.Mut} (frameKind: U8, subKind: U8, argIdx: U8, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  if frameKind == 9 then
    if subKind == 1 then
      Stigi.Emit.writeLiteral(15, ") + (uint64_t)(", outCap, output, writeOffset)
    else if subKind == 2 then
      Stigi.Emit.writeLiteral(15, ") - (uint64_t)(", outCap, output, writeOffset)
    else if subKind == 3 then
      Stigi.Emit.writeLiteral(15, ") * (uint64_t)(", outCap, output, writeOffset)
    else if subKind == 4 then
      Stigi.Emit.writeLiteral(15, ") & (uint64_t)(", outCap, output, writeOffset)
    else if subKind == 5 then
      Stigi.Emit.writeLiteral(15, ") | (uint64_t)(", outCap, output, writeOffset)
    else
      Stigi.Emit.writeLiteral(15, ") ^ (uint64_t)(", outCap, output, writeOffset)
  else if frameKind == 10 then
    if subKind == 1 then
      Stigi.Emit.writeLiteral(17, ") << ((uint64_t)(", outCap, output, writeOffset)
    else
      Stigi.Emit.writeLiteral(17, ") >> ((uint64_t)(", outCap, output, writeOffset)
  else if frameKind == 11 then
    if subKind == 1 then
      Stigi.Emit.writeLiteral(15, ") + (uint32_t)(", outCap, output, writeOffset)
    else if subKind == 2 then
      Stigi.Emit.writeLiteral(15, ") - (uint32_t)(", outCap, output, writeOffset)
    else if subKind == 3 then
      Stigi.Emit.writeLiteral(15, ") * (uint32_t)(", outCap, output, writeOffset)
    else if subKind == 4 then
      Stigi.Emit.writeLiteral(15, ") & (uint32_t)(", outCap, output, writeOffset)
    else if subKind == 5 then
      Stigi.Emit.writeLiteral(15, ") | (uint32_t)(", outCap, output, writeOffset)
    else
      Stigi.Emit.writeLiteral(15, ") ^ (uint32_t)(", outCap, output, writeOffset)
  else if frameKind == 12 then
    if subKind == 1 then
      Stigi.Emit.writeLiteral(17, ") << ((uint32_t)(", outCap, output, writeOffset)
    else
      Stigi.Emit.writeLiteral(17, ") >> ((uint32_t)(", outCap, output, writeOffset)
  else if frameKind == 13 then
    Stigi.Emit.writeLiteral(14, "), (uint32_t)(", outCap, output, writeOffset)
  else if frameKind == 15 then
    if argIdx == 0 then
      Stigi.Emit.writeLiteral(6, ") == (", outCap, output, writeOffset)
    else
      Stigi.Emit.writeLiteral(17, ")) ? 0 : ((void)(", outCap, output, writeOffset)
  else if frameKind == 16 then
    if argIdx == 0 then
      Stigi.Emit.writeLiteral(4, "), (", outCap, output, writeOffset)
    else
      Stigi.Emit.writeLiteral(17, ")) ? 0 : ((void)(", outCap, output, writeOffset)
  else if frameKind == 17 then
    Stigi.Emit.emitPosixComma(subKind, argIdx, outCap, output, writeOffset)
  else
    Stigi.Emit.writeLiteral(2, ", ", outCap, output, writeOffset)
