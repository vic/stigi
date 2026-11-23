// prompt: emit a numeric literal token with ISO C11 pedantic-clean unsigned suffix where needed.
def Stigi.Emit.emitNumberLiteral {Stigi.Mut} (srcCap: U64, srcBytes: Array[U8](srcCap), litBeg: U64, litFin: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(litBeg, litFin), lessOrEqual(litFin, srcCap), below(srcCap, 0x100000000), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  val pos: U64 = Stigi.Print.writeSlice(srcCap, srcBytes, litBeg, litFin, outCap, output, writeOffset)
  val litLen: U64 = litFin - litBeg
  val nextPos: U64 = litBeg + 1
  if litLen >= 2 && nextPos < litFin && srcBytes(litBeg) == 48 && (srcBytes(nextPos) == 120 || srcBytes(nextPos) == 88) then
    if litLen > 10 then
      Stigi.Emit.writeLiteral(3, "ULL", outCap, output, pos)
    else
      Stigi.Emit.writeLiteral(1, "U", outCap, output, pos)
  else if litLen >= 10 then
    Stigi.Emit.writeLiteral(3, "ULL", outCap, output, pos)
  else
    pos
