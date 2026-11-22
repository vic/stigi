// prompt: emit an S52 debug line directive pointing at the stored tree unit path.
pub def Stigi.Emit.writeLineDirective {region: Stigi.Region, Stigi.Mut} (lineNumber: U64, nameLen: U64, name: Array[U8](nameLen), outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(nameLen, 0x100000000), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  var pos: U64 = Stigi.Emit.writeLiteral(6, "#line ", outCap, output, writeOffset)
  pos = Stigi.Emit.writeUnsignedDecimal{region}(lineNumber, outCap, output, pos)
  pos = Stigi.Emit.writeLiteral(2, " \"", outCap, output, pos)
  pos = Stigi.Emit.writeSourcePath(nameLen, name, outCap, output, pos)
  pos = Stigi.Emit.writeByte(outCap, output, pos, 34)
  Stigi.Emit.writeByte(outCap, output, pos, 10)
