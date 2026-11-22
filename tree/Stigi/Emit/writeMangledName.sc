// prompt: write a mangled C symbol with Stigi_ prefix replacing dots with underscores.
pub def Stigi.Emit.writeMangledName {Stigi.Mut} (nameLen: U64, name: Array[U8](nameLen), outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(nameLen, 0x100000000), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  var pos: U64 = Stigi.Emit.writeLiteral(6, "Stigi_", outCap, output, writeOffset)
  var index: U64 = 0
  while index < nameLen do
    @<lessOrEqual(index, nameLen), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(nameLen - index)>
    val rawByte: U8 = name(index)
    val mappedByte: U8 = if rawByte == 46 then 95 else rawByte
    pos = Stigi.Emit.writeByte(outCap, output, pos, mappedByte)
    index = index + 1
  pos
