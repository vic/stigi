// prompt: append a literal byte array into a bounded C output buffer.
pub def Stigi.Emit.writeLiteral {Stigi.Mut} (textLen: U64, text: Array[U8](textLen), outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  Stigi.Print.writeSlice(textLen, text, 0, textLen, outCap, output, writeOffset)
