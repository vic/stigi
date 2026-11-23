// prompt: emit the C separator for a POSIX extern argument boundary.
def Stigi.Emit.emitPosixComma {Stigi.Mut} (subKind: U8, argIdx: U8, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  if subKind == 1 then
    if argIdx == 0 then
      Stigi.Emit.writeLiteral(18, "), (const char *)(", outCap, output, writeOffset)
    else if argIdx == 1 then
      Stigi.Emit.writeLiteral(9, "), (int)(", outCap, output, writeOffset)
    else
      Stigi.Emit.writeLiteral(12, "), (mode_t)(", outCap, output, writeOffset)
  else if subKind == 2 then
    if argIdx == 0 then
      Stigi.Emit.writeLiteral(12, "), (void *)(", outCap, output, writeOffset)
    else
      Stigi.Emit.writeLiteral(12, "), (size_t)(", outCap, output, writeOffset)
  else if subKind == 3 then
    if argIdx == 0 then
      Stigi.Emit.writeLiteral(18, "), (const void *)(", outCap, output, writeOffset)
    else
      Stigi.Emit.writeLiteral(12, "), (size_t)(", outCap, output, writeOffset)
  else if subKind == 4 then
    if argIdx == 0 then
      Stigi.Emit.writeLiteral(11, "), (off_t)(", outCap, output, writeOffset)
    else
      Stigi.Emit.writeLiteral(9, "), (int)(", outCap, output, writeOffset)
  else if subKind == 7 then
    Stigi.Emit.writeLiteral(29, "), (struct rusage *)(void *)(", outCap, output, writeOffset)
  else
    Stigi.Emit.writeLiteral(31, "), (struct timespec *)(void *)(", outCap, output, writeOffset)
