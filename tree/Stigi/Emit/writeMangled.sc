// prompt: write Stigi_ followed by a qualified name with dots mangled to underscores.
def Stigi.Emit.writeMangled {writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), nameStart: U64, nameEnd: U64) <lessOrEqual(nameStart, nameEnd), lessOrEqual(nameEnd, srcCap)>: Unit =
  Stigi.Emit.writeLit{writer}(outCap, outBuf, 6, "Stigi_")
  var pos: U64 = nameStart
  while pos < nameEnd do
    @<lessOrEqual(nameStart, pos), lessOrEqual(pos, nameEnd)>
    val curByte: U8 = src(pos)
    val outByte: U8 = if curByte == 46 then 95 else curByte
    writer.putByte(outCap, outBuf, outByte)
    pos = pos + 1
