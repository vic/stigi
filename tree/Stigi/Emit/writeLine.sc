// prompt: append a literal string followed by a newline byte into the C output buffer.
pub def Stigi.Emit.writeLine {Stigi.Mut} (textLen: U64, text: Array[U8](textLen), outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val afterText: U64 = Stigi.Emit.writeLiteral(textLen, text, outCap, output, writeOffset)
  Stigi.Emit.writeByte(outCap, output, afterText, 10)
