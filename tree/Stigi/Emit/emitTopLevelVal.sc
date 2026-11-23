// prompt: emit a C static const definition for a top-level L0 val unit.
pub def Stigi.Emit.emitTopLevelVal {Stigi.Mut} (indexCap: U64, indexBytes: Array[U8](indexCap), nameStart: U64, nameEnd: U64, srcCap: U64, srcBytes: Array[U8](srcCap), tokCap: U64, tokKind: Array[U8](tokCap), tokStart: Array[U64](tokCap), tokEnd: Array[U64](tokCap), tokCount: U64, returnTok: U64, hasReturnTok: Bool, bodyTok: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(nameStart, nameEnd), lessOrEqual(nameEnd, indexCap), below(indexCap, 0x100000000), lessOrEqual(tokCount, tokCap), below(tokCap, 65536), below(srcCap, 0x100000000), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  var pos: U64 = Stigi.Emit.writeLiteral(13, "static const ", outCap, output, writeOffset)
  val isUnsignedLong: Bool = if hasReturnTok && returnTok < tokCount then Stigi.Emit.tokenEquals(srcCap, srcBytes, tokCap, tokStart, tokEnd, returnTok, 3, "U64") else false
  if hasReturnTok && returnTok < tokCount then
    pos = Stigi.Emit.emitTypeName(srcCap, srcBytes, tokCap, tokStart, tokEnd, tokCount, returnTok, outCap, output, pos)
  else
    pos = Stigi.Emit.writeLiteral(8, "uint64_t", outCap, output, pos)
  pos = Stigi.Emit.writeLiteral(1, " ", outCap, output, pos)
  pos = Stigi.Emit.writeSliceMangled(indexCap, indexBytes, nameStart, nameEnd, true, outCap, output, pos)
  pos = Stigi.Emit.writeLiteral(3, " = ", outCap, output, pos)
  if bodyTok < tokCount then
    val litBeg: U64 = tokStart(bodyTok)
    val litFin: U64 = tokEnd(bodyTok)
    if litBeg <= litFin && litFin <= srcCap then
      pos = Stigi.Emit.writeSliceMangled(srcCap, srcBytes, litBeg, litFin, false, outCap, output, pos)
    else
      pos = Stigi.Emit.writeLiteral(1, "0", outCap, output, pos)
    if isUnsignedLong && tokKind(bodyTok) == 3 then
      pos = Stigi.Emit.writeLiteral(3, "ULL", outCap, output, pos)
    else
      ()
  else
    pos = Stigi.Emit.writeLiteral(1, "0", outCap, output, pos)
  Stigi.Emit.writeLine(1, ";", outCap, output, pos)
