// prompt: write a stored-tree relative path replacing dots with slashes and appending .sc.
pub def Stigi.Emit.writeSourcePath {Stigi.Mut} (nameLen: U64, name: Array[U8](nameLen), outCap: U64, output: Array[U8](outCap), writeOffset: U64) <below(nameLen, 0x100000000), lessOrEqual(writeOffset, outCap), below(outCap, 0x100000000)>: U64 <lessOrEqual(writeOffset, result), lessOrEqual(result, outCap)> =
  var pos: U64 = writeOffset
  var index: U64 = 0
  while index < nameLen do
    @<lessOrEqual(index, nameLen), lessOrEqual(writeOffset, pos), lessOrEqual(pos, outCap), decreases(nameLen - index)>
    val rawByte: U8 = name(index)
    val mappedByte: U8 = if rawByte == 46 then 47 else rawByte
    pos = Stigi.Emit.writeByte(outCap, output, pos, mappedByte)
    index = index + 1
  Stigi.Emit.writeLiteral(3, ".sc", outCap, output, pos)
