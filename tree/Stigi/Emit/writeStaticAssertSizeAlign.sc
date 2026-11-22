// prompt: emit compile-time C11 static assertions locking enum size and alignment.
pub def Stigi.Emit.writeStaticAssertSizeAlign {region: Stigi.Region, Stigi.Mut} (nameLen: U64, name: Array[U8](nameLen), sizeBytes: U64, alignBytes: U64, outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(nameLen, 0x100000000), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  var pos: U64 = Stigi.Emit.writeLiteral(22, "_Static_assert(sizeof(", outCap, output, writeOffset)
  pos = Stigi.Emit.writeMangledName(nameLen, name, outCap, output, pos)
  pos = Stigi.Emit.writeLiteral(5, ") == ", outCap, output, pos)
  pos = Stigi.Emit.writeUnsignedDecimal{region}(sizeBytes, outCap, output, pos)
  pos = Stigi.Emit.writeLiteral(15, "ULL, \"sizeOf\");", outCap, output, pos)
  pos = Stigi.Emit.writeByte(outCap, output, pos, 10)
  pos = Stigi.Emit.writeLiteral(24, "_Static_assert(_Alignof(", outCap, output, pos)
  pos = Stigi.Emit.writeMangledName(nameLen, name, outCap, output, pos)
  pos = Stigi.Emit.writeLiteral(5, ") == ", outCap, output, pos)
  pos = Stigi.Emit.writeUnsignedDecimal{region}(alignBytes, outCap, output, pos)
  pos = Stigi.Emit.writeLiteral(16, "ULL, \"alignOf\");", outCap, output, pos)
  Stigi.Emit.writeByte(outCap, output, pos, 10)
