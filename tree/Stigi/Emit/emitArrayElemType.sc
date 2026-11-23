// prompt: emit the C pointer type for an L0 Array element token.
def Stigi.Emit.emitArrayElemType {Stigi.Mut} (srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, typeTok: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(tokCount, tokCap), below(tokCap, 65536), below(typeTok, tokCount), below(srcCap, 0x100000000), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val elemTok: U64 = typeTok + 2
  if elemTok < tokCount then
    val elemStart: U64 = tokStart(elemTok)
    val elemEnd: U64 = tokEnd(elemTok)
    if elemStart <= elemEnd && elemEnd <= srcCap then
      val pos: U64 = Stigi.Emit.emitTypeSlice(srcCap, srcBytes, elemStart, elemEnd, outCap, output, writeOffset)
      Stigi.Emit.writeLiteral(2, " *", outCap, output, pos)
    else
      Stigi.Emit.writeLiteral(6, "void *", outCap, output, writeOffset)
  else
    Stigi.Emit.writeLiteral(6, "void *", outCap, output, writeOffset)
