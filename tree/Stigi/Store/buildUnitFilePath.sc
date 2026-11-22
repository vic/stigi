// prompt: build the stored .sc file path for a unit inside directory dirBuf[dirStart..dirEnd].
pub def Stigi.Store.buildUnitFilePath {Stigi.Mut} (dirCap: U64, dirBuf: Array[U8](dirCap), dirStart: U64, dirEnd: U64, nameCap: U64, nameBuf: Array[U8](nameCap), nameStart: U64, nameEnd: U64, outCap: U64, outBuf: Array[U8](outCap)) <lessOrEqual(dirStart, dirEnd), lessOrEqual(dirEnd, dirCap), below(dirCap, 0x100000000), lessOrEqual(nameStart, nameEnd), lessOrEqual(nameEnd, nameCap), below(nameCap, 0x100000000), below(outCap, 0x100000000)>: U64 <lessOrEqual(result, outCap)> =
  var pos: U64 = Stigi.Print.writeSlice(dirCap, dirBuf, dirStart, dirEnd, outCap, outBuf, 0)
  pos = Stigi.Emit.writeByte(outCap, outBuf, pos, 47)
  var cursor: U64 = nameStart
  while cursor < nameEnd do
    @<lessOrEqual(nameStart, cursor), lessOrEqual(cursor, nameEnd), lessOrEqual(pos, outCap), decreases(nameEnd - cursor)>
    val rawByte: U8 = nameBuf(cursor)
    val mappedByte: U8 = if rawByte == 46 then 47 else rawByte
    pos = Stigi.Emit.writeByte(outCap, outBuf, pos, mappedByte)
    cursor = cursor + 1
  Stigi.Emit.writeLiteral(3, ".sc", outCap, outBuf, pos)
