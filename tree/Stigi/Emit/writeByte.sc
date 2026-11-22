// prompt: append one byte into a bounded C output buffer.
pub def Stigi.Emit.writeByte {Stigi.Mut} (outCap: U64, output: Array[U8](outCap), writeOffset: U64, byteVal: U8) <lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  if writeOffset < outCap then
    output(writeOffset) := byteVal
    writeOffset + 1
  else
    writeOffset
