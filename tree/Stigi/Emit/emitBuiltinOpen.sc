// prompt: emit the opening C expression fragment for a classified builtin or POSIX extern call.
pub def Stigi.Emit.emitBuiltinOpen {Stigi.Mut} (frameKind: U8, subKind: U8, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  if frameKind == 3 || frameKind == 9 || frameKind == 10 then
    Stigi.Emit.writeLiteral(12, "((uint64_t)(", outCap, output, writeOffset)
  else if frameKind == 4 then
    Stigi.Emit.writeLiteral(11, "((uint8_t)(", outCap, output, writeOffset)
  else if frameKind == 5 then
    Stigi.Emit.writeLiteral(12, "((uint32_t)(", outCap, output, writeOffset)
  else if frameKind == 6 then
    Stigi.Emit.writeLiteral(11, "((int32_t)(", outCap, output, writeOffset)
  else if frameKind == 7 then
    Stigi.Emit.writeLiteral(11, "((int64_t)(", outCap, output, writeOffset)
  else if frameKind == 8 then
    Stigi.Emit.writeLiteral(25, "stigi_u64_mix((uint64_t)(", outCap, output, writeOffset)
  else if frameKind == 11 || frameKind == 12 then
    Stigi.Emit.writeLiteral(23, "((uint32_t)((uint32_t)(", outCap, output, writeOffset)
  else if frameKind == 13 then
    Stigi.Emit.writeLiteral(26, "stigi_u32_rotr((uint32_t)(", outCap, output, writeOffset)
  else if frameKind == 15 then
    Stigi.Emit.writeLiteral(3, "(((", outCap, output, writeOffset)
  else if frameKind == 17 then
    if subKind == 1 then
      Stigi.Emit.writeLiteral(23, "((int32_t)openat((int)(", outCap, output, writeOffset)
    else if subKind == 2 then
      Stigi.Emit.writeLiteral(21, "((int64_t)read((int)(", outCap, output, writeOffset)
    else if subKind == 3 then
      Stigi.Emit.writeLiteral(22, "((int64_t)write((int)(", outCap, output, writeOffset)
    else if subKind == 4 then
      Stigi.Emit.writeLiteral(22, "((int64_t)lseek((int)(", outCap, output, writeOffset)
    else if subKind == 5 then
      Stigi.Emit.writeLiteral(22, "((int32_t)close((int)(", outCap, output, writeOffset)
    else if subKind == 6 then
      Stigi.Emit.writeLiteral(32, "((int32_t)unlink((const char *)(", outCap, output, writeOffset)
    else if subKind == 7 then
      Stigi.Emit.writeLiteral(26, "((int32_t)getrusage((int)(", outCap, output, writeOffset)
    else
      Stigi.Emit.writeLiteral(36, "((int32_t)clock_gettime((clockid_t)(", outCap, output, writeOffset)
  else
    writeOffset
