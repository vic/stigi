// prompt: render a parsed unit signature header line through Stigi.Emit.Writer.
pub def Stigi.Print.printHeader {writer: Stigi.Emit.Writer, Stigi.Mut} (outCap: Stigi.EmitCap, outBuf: Array[U8](outCap), srcCap: Stigi.ByteCap, src: Array[U8](srcCap), header: Stigi.Ast.UnitHeader) <below(0, outCap)>: Bool =
  val UnitHeader(declKind, isPub, nameStart, nameEnd, _, _, _) = header
  if isPub then writer.putSlice(outCap, outBuf, 4, "pub ", 0, 4) else ()
  Stigi.Print.writeDeclKeyword{writer}(outCap, outBuf, declKind)
  if nameEnd <= srcCap then
    writer.putSlice(outCap, outBuf, srcCap, src, nameStart, nameEnd)
  else ()
  writer.putByte(outCap, outBuf, 10)
  !writer.didOverflow()
