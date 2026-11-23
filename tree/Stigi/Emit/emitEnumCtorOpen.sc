// prompt: emit a C compound literal opening for a nullary or payload-bearing enum constructor.
def Stigi.Emit.emitEnumCtorOpen {region: Stigi.Region, Stigi.Mut} (indexCap: U64, indexBytes: Array[U8](indexCap), enumBeg: U64, enumFin: U64, localTag: U64, hasArgs: Bool, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <lessOrEqual(enumBeg, enumFin), lessOrEqual(enumFin, indexCap), below(indexCap, 0x100000000), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  var pos: U64 = Stigi.Emit.writeLiteral(2, "((", outCap, output, writeOffset)
  pos = Stigi.Emit.writeSliceMangled(indexCap, indexBytes, enumBeg, enumFin, true, outCap, output, pos)
  pos = Stigi.Emit.writeLiteral(10, "){ .tag = ", outCap, output, pos)
  pos = Stigi.Emit.writeUnsignedDecimal{region}(localTag, outCap, output, pos)
  if hasArgs then
    pos = Stigi.Emit.writeLiteral(12, "U, .u = { .c", outCap, output, pos)
    pos = Stigi.Emit.writeUnsignedDecimal{region}(localTag, outCap, output, pos)
    Stigi.Emit.writeLiteral(5, " = { ", outCap, output, pos)
  else
    Stigi.Emit.writeLiteral(4, "U })", outCap, output, pos)
