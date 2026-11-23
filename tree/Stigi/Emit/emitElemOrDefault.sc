// prompt: emit a type slice or uint64_t default when the element type span is empty.
def Stigi.Emit.emitElemOrDefault {Stigi.Mut} (srcCap: U64, srcBytes: Array[U8](srcCap), elemBeg: U64, elemFin: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(srcCap, 0x100000000), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  if elemBeg < elemFin && elemFin <= srcCap then
    Stigi.Emit.emitTypeSlice(srcCap, srcBytes, elemBeg, elemFin, outCap, output, writeOffset)
  else
    Stigi.Emit.writeLiteral(8, "uint64_t", outCap, output, writeOffset)
